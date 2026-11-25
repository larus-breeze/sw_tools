//! Linux SocketCAN command-line firmware uploader.
use can_update::{CanFrame, FirmwareSource, TransmittingDevice, TransmittingState};
use std::{
    env,
    error::Error,
    ffi::CString,
    fs::File,
    io::{Read, Seek, SeekFrom},
    mem,
    os::raw::{c_char, c_int, c_uint, c_void},
    time::Instant,
};

const AF_CAN: c_int = 29;
const SOCK_RAW: c_int = 3;
const CAN_RAW: c_int = 1;
const POLLIN: i16 = 0x0001;
const CAN_EFF_FLAG: u32 = 0x8000_0000;
const CAN_RTR_FLAG: u32 = 0x4000_0000;
const CAN_ERR_FLAG: u32 = 0x2000_0000;
const CAN_SFF_MASK: u32 = 0x7ff;
const CAN_EFF_MASK: u32 = 0x1fff_ffff;

#[repr(C)]
struct SockAddrCan {
    family: u16,
    ifindex: c_int,
    addr: [u8; 8],
}
#[repr(C)]
#[derive(Clone, Copy)]
struct LinuxCanFrame {
    can_id: u32,
    can_dlc: u8,
    pad: u8,
    res0: u8,
    len8_dlc: u8,
    data: [u8; 8],
}
#[repr(C)]
struct PollFd {
    fd: c_int,
    events: i16,
    revents: i16,
}
unsafe extern "C" {
    fn socket(domain: c_int, ty: c_int, protocol: c_int) -> c_int;
    fn bind(fd: c_int, addr: *const c_void, len: u32) -> c_int;
    fn if_nametoindex(name: *const c_char) -> c_uint;
    fn poll(fds: *mut PollFd, nfds: usize, timeout: c_int) -> c_int;
    fn read(fd: c_int, buf: *mut c_void, count: usize) -> isize;
    fn write(fd: c_int, buf: *const c_void, count: usize) -> isize;
    fn close(fd: c_int) -> c_int;
}
struct CanSocket(c_int);
impl Drop for CanSocket {
    fn drop(&mut self) {
        unsafe {
            close(self.0);
        }
    }
}
impl CanSocket {
    fn open(interface: &str) -> Result<Self, Box<dyn Error>> {
        let name = CString::new(interface)?;
        let index = unsafe { if_nametoindex(name.as_ptr()) };
        if index == 0 {
            return Err(format!("CAN interface '{interface}' not found").into());
        }
        let fd = unsafe { socket(AF_CAN, SOCK_RAW, CAN_RAW) };
        if fd < 0 {
            return Err(std::io::Error::last_os_error().into());
        }
        let sock = Self(fd);
        let addr = SockAddrCan {
            family: AF_CAN as u16,
            ifindex: index as c_int,
            addr: [0; 8],
        };
        if unsafe {
            bind(
                fd,
                &addr as *const _ as *const c_void,
                mem::size_of::<SockAddrCan>() as u32,
            )
        } < 0
        {
            return Err(std::io::Error::last_os_error().into());
        }
        Ok(sock)
    }
    fn send(&self, frame: CanFrame) -> Result<(), Box<dyn Error>> {
        if frame.id > CAN_EFF_MASK || frame.len > 8 {
            return Err("Invalid CAN ID or payload length".into());
        }
        let can_id = if frame.id > CAN_SFF_MASK {
            frame.id | CAN_EFF_FLAG
        } else {
            frame.id
        };
        let raw = LinuxCanFrame {
            can_id,
            can_dlc: frame.len,
            pad: 0,
            res0: 0,
            len8_dlc: 0,
            data: frame.data,
        };
        let n = unsafe {
            write(
                self.0,
                &raw as *const _ as *const c_void,
                mem::size_of::<LinuxCanFrame>(),
            )
        };
        if n != mem::size_of::<LinuxCanFrame>() as isize {
            if n < 0 {
                return Err(std::io::Error::last_os_error().into());
            }
            return Err("Incomplete CAN frame write".into());
        }
        Ok(())
    }
    fn recv(&self, wait_ms: c_int) -> Result<Option<CanFrame>, Box<dyn Error>> {
        let mut pfd = PollFd {
            fd: self.0,
            events: POLLIN,
            revents: 0,
        };
        let ready = unsafe { poll(&mut pfd, 1, wait_ms) };
        if ready < 0 {
            return Err(std::io::Error::last_os_error().into());
        }
        if ready == 0 {
            return Ok(None);
        }
        if pfd.revents & POLLIN == 0 {
            return Err("Socket poll error".into());
        }
        let mut raw = LinuxCanFrame {
            can_id: 0,
            can_dlc: 0,
            pad: 0,
            res0: 0,
            len8_dlc: 0,
            data: [0; 8],
        };
        let n = unsafe {
            read(
                self.0,
                &mut raw as *mut _ as *mut c_void,
                mem::size_of::<LinuxCanFrame>(),
            )
        };
        if n != mem::size_of::<LinuxCanFrame>() as isize {
            return Err("Invalid CAN frame read".into());
        }
        if raw.can_id & (CAN_RTR_FLAG | CAN_ERR_FLAG) != 0 || raw.can_dlc > 8 {
            return Ok(None);
        }
        let id = raw.can_id
            & if raw.can_id & CAN_EFF_FLAG != 0 {
                CAN_EFF_MASK
            } else {
                CAN_SFF_MASK
            };
        Ok(Some(CanFrame {
            id,
            len: raw.can_dlc,
            data: raw.data,
        }))
    }
}
struct FileSource(File);
impl FirmwareSource for FileSource {
    type Error = std::io::Error;
    fn read(&mut self, offset: u32, out: &mut [u8]) -> Result<(), Self::Error> {
        self.0.seek(SeekFrom::Start(offset as u64))?;
        self.0.read_exact(out)
    }
}
fn number(s: &str) -> Result<u32, Box<dyn Error>> {
    Ok(
        if let Some(h) = s.strip_prefix("0x").or_else(|| s.strip_prefix("0X")) {
            u32::from_str_radix(h, 16)?
        } else {
            s.parse()?
        },
    )
}
fn usage() -> &'static str {
    "Usage: larus-can-upload --interface can0 --transmitting-id 0x100 --receiving-id 0x101 --uuid 0x12345678 firmware.bin\n\
     IDs and UUID accept decimal or 0x-prefixed hexadecimal. Firmware size max 16777215."
}
fn main() {
    if let Err(e) = run() {
        eprintln!("Upload failed: {e}");
        std::process::exit(1);
    }
}
fn run() -> Result<(), Box<dyn Error>> {
    let mut interface = None;
    let mut transmitting_id = None;
    let mut receiving_id = None;
    let mut uuid = None;
    let mut path = None;
    let mut args = env::args().skip(1);
    while let Some(arg) = args.next() {
        match arg.as_str() {
            "-h" | "--help" => {
                println!("{}", usage());
                return Ok(());
            }
            "--interface" => interface = Some(args.next().ok_or("Missing interface")?),
            "--transmitting-id" => {
                transmitting_id = Some(number(&args.next().ok_or("Missing transmitting ID")?)?)
            }
            "--receiving-id" => {
                receiving_id = Some(number(&args.next().ok_or("Missing receiving ID")?)?)
            }
            "--uuid" => uuid = Some(number(&args.next().ok_or("Missing UUID")?)?),
            _ if !arg.starts_with('-') && path.is_none() => path = Some(arg),
            _ => return Err(format!("Unknown or duplicate argument: {arg}\n{}", usage()).into()),
        }
    }
    let interface = interface.ok_or(usage())?;
    let transmitting_id = transmitting_id.ok_or(usage())?;
    let receiving_id = receiving_id.ok_or(usage())?;
    let uuid = uuid.ok_or(usage())?;
    let path = path.ok_or(usage())?;
    if transmitting_id > CAN_EFF_MASK
        || receiving_id > CAN_EFF_MASK
        || transmitting_id == receiving_id
    {
        return Err("Invalid or identical CAN IDs".into());
    }
    let file = File::open(&path)?;
    let size = file.metadata()?.len();
    if size == 0 || size > 0x00ff_ffff {
        return Err("Firmware must be nonempty and <= 16777215 bytes".into());
    }
    let mut source = FileSource(file);
    let mut transmitting = TransmittingDevice::new(transmitting_id, receiving_id);
    transmitting
        .start(&mut source, uuid, size as u32)
        .map_err(|e| format!("Firmware preparation: {e:?}"))?;
    let socket = CanSocket::open(&interface)?;
    let start = Instant::now();
    let mut last_state = TransmittingState::Idle;
    println!("Uploading {path} ({size} bytes) over {interface}, UUID 0x{uuid:08x}");
    loop {
        let now = || start.elapsed().as_millis() as u32;
        transmitting
            .tick(now())
            .map_err(|e| format!("Protocol: {e:?}"))?;
        match transmitting.state() {
            TransmittingState::Completed => {
                println!("Upload completed successfully (RD.5 received).");
                return Ok(());
            }
            TransmittingState::Failed => return Err("Sender entered failed state".into()),
            state => {
                if state != last_state {
                    match state {
                        TransmittingState::SendingBlockInit => println!("Starting next block ..."),
                        TransmittingState::WaitingUploadAck => {
                            println!("Waiting for final image verification ...")
                        }
                        _ => {}
                    }
                    last_state = state;
                }
            }
        }
        if let Some(frame) = transmitting
            .next_frame()
            .map_err(|e| format!("Next frame: {e:?}"))?
        {
            socket.send(frame)?;
            transmitting
                .frame_sent(now())
                .map_err(|e| format!("Frame sent: {e:?}"))?;
            continue;
        }
        if let Some(frame) = socket.recv(10)? {
            if frame.id == receiving_id {
                transmitting
                    .on_frame(frame, now(), &mut source)
                    .map_err(|e| format!("Device response: {e:?}"))?;
            }
        }
    }
}
