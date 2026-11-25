#![no_std]
#![no_main]

use core::num::{NonZeroU16, NonZeroU8};
use cortex_m_rt::entry;
use defmt_rtt as _;
use fdcan::{
    config::NominalBitTiming,
    filter::{StandardFilter, StandardFilterSlot},
    frame::{FrameFormat, TxFrameHeader},
    id::{Id, StandardId},
};
use hal::{
    gpio::{GpioExt as _, Speed},
    pac,
    prelude::*,
    rcc::{self, rec},
};
use can_update::CanFrame;
use panic_probe as _;
use stm32h7_test::{CanPort, TestReceiver};
use stm32h7xx_hal as hal;
use stm32h7xx_hal::flash::FlashExt;

const TRANSMITTING_ID: u32 = 0x100;
const RECEIVING_ID: u32 = 0x101;
const UUID: u32 = 0x1234_5678;

struct SocketCanPort<F> {
    can: F,
}

impl<F> SocketCanPort<F> {
    fn new(can: F) -> Self {
        Self { can }
    }
}

// The HAL wraps FDCAN1 in its `Can` storage type.
type Can1 = fdcan::FdCan<hal::can::Can<pac::FDCAN1>, fdcan::NormalOperationMode>;

impl CanPort for SocketCanPort<Can1> {
    type Error = ();

    fn receive(&mut self) -> Result<Option<CanFrame>, Self::Error> {
        // fdcan 0.2.1: receive0() does NOT return Option.
        // Check FIFO0 first to avoid calling receive0() when empty.
        let (_, _, rx0, _) = self.can.split_by_ref();
        if rx0.rx_fifo_is_empty() {
            return Ok(None);
        }

        let mut bytes = [0u8; 64];
        let info = match rx0.receive(&mut bytes) {
            Ok(fdcan::ReceiveOverrun::NoOverrun(info)) => info,
            Ok(fdcan::ReceiveOverrun::Overrun(info)) => {
                defmt::warn!("FDCAN FIFO0 overrun");
                info
            }
            Err(nb::Error::WouldBlock) => return Ok(None),
            Err(nb::Error::Other(_)) => return Err(()),
        };
        let id = match info.id {
            Id::Standard(id) => id.as_raw() as u32,
            Id::Extended(_) => return Ok(None),
        };
        if info.len > 8 || info.frame_format != FrameFormat::Standard {
            return Ok(None);
        }
        let mut data = [0u8; 8];
        data[..info.len as usize].copy_from_slice(&bytes[..info.len as usize]);
        Ok(Some(CanFrame {
            id,
            len: info.len,
            data,
        }))
    }

    fn send(&mut self, frame: CanFrame) -> Result<(), Self::Error> {
        let id = StandardId::new(frame.id as u16).ok_or(())?;
        let header = TxFrameHeader {
            len: frame.len,
            id: id.into(),
            frame_format: FrameFormat::Standard,
            bit_rate_switching: false,
            marker: None,
        };
        // Do not allow fdcan::transmit() to replace a pending frame.
        // In fdcan 0.2.1, Ok(None) means queued without replacing anything.
        for _ in 0..100_000 {
            let (_, tx, _, _) = self.can.split_by_ref();
            if tx.tx_queue_is_full() {
                continue;
            }
            match tx.transmit(header, &frame.data[..frame.len as usize]) {
                Ok(None) => return Ok(()),
                Ok(Some(())) => {
                    defmt::error!("FDCAN replaced pending TX frame");
                    return Err(());
                }
                Err(nb::Error::WouldBlock) => {}
                Err(nb::Error::Other(_)) => return Err(()),
            }
        }
        Err(())
    }
}

#[entry]
fn main() -> ! {
    let dp = pac::Peripherals::take().unwrap();
    let (_bank1, bank2) = dp.FLASH.split();
    let mut bank2 = bank2.expect("Flash Bank 2 nicht vorhanden");

    {
        let flash = bank2.unlocked();

        defmt::info!(
            "Bank 2: base={=usize:#x}, size={=usize}",
            flash.address(),
            flash.len()
        );
        let data = &flash.read_all()[..32];

        for (i, chunk) in data.chunks(4).enumerate() {
            defmt::info!("Flash +{=usize}: {=[u8]:x}", i * 4, chunk);
        }
    }

    let mut cp = cortex_m::Peripherals::take().unwrap();
    defmt::info!("LARUS STM32H743 CAN update test");

    let pwrcfg = dp.PWR.constrain().freeze();
    let ccdr = dp
        .RCC
        .constrain()
        .use_hse(16.MHz())
        .sys_ck(400.MHz())
        .pll1_strategy(rcc::PllConfigStrategy::Iterative)
        .pll1_q_ck(50.MHz())
        .freeze(pwrcfg, &dp.SYSCFG);
    assert_eq!(ccdr.clocks.pll1_q_ck().unwrap().raw(), 50_000_000);
    let fdcan_prec = ccdr
        .peripheral
        .FDCAN
        .kernel_clk_mux(rec::FdcanClkSel::Pll1Q);

    let gpiob = dp.GPIOB.split(ccdr.peripheral.GPIOB);
    let rx = gpiob.pb8.into_alternate().speed(Speed::VeryHigh);
    let tx = gpiob.pb9.into_alternate().speed(Speed::VeryHigh);
    let mut can = dp.FDCAN1.fdcan(tx, rx, fdcan_prec);
    can.set_protocol_exception_handling(false);
    can.set_nominal_bit_timing(NominalBitTiming {
        prescaler: NonZeroU16::new(5).unwrap(),
        seg1: NonZeroU8::new(7).unwrap(),
        seg2: NonZeroU8::new(2).unwrap(),
        sync_jump_width: NonZeroU8::new(2).unwrap(),
    });
    can.set_standard_filter(
        StandardFilterSlot::_0,
        StandardFilter::accept_all_into_fifo0(),
    );
    let mut can = SocketCanPort::new(can.into_normal());
    defmt::info!("FDCAN1 PB8/PB9, 1 Mbit/s, standard frames");

    // CYCCNT is free-running; sample it frequently to handle wraparound.
    cp.DCB.enable_trace();
    cp.DWT.enable_cycle_counter();
    let mut last_cycles = cortex_m::peripheral::DWT::cycle_count();
    let mut fractional_cycles = 0u32;
    let mut now_ms = 0u32;
    let mut receiver = TestReceiver::new(UUID, TRANSMITTING_ID, RECEIVING_ID);
    loop {
        let cycles = cortex_m::peripheral::DWT::cycle_count();
        fractional_cycles += cycles.wrapping_sub(last_cycles);
        last_cycles = cycles;
        if fractional_cycles >= 400_000 {
            now_ms = now_ms.wrapping_add(fractional_cycles / 400_000);
            fractional_cycles %= 400_000;
        }
        if let Err(e) = receiver.poll(&mut can, &mut bank2, now_ms) {
            defmt::error!("CAN polling error: {:?}", defmt::Debug2Format(&e));
        }
    }
}
