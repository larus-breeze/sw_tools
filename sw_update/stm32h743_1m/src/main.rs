#![no_main]
#![no_std]

use {defmt_rtt as _, panic_probe as _};
use cortex_m_rt::entry;
use stm32h7xx_hal::{pac, prelude::*};

/*#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    cortex_m::peripheral::SCB::sys_reset();
}*/

include!("../../common/meta_v1.rs");

// We simulate the loading of the image by flashing it with the debugger to the upper area.
//
// $ mon program image.bin 0x08100000
//
// Then start test app
#[entry]
fn main() -> ! {
    let mut cp = cortex_m::Peripherals::take().unwrap();
    let dp = pac::Peripherals::take().unwrap();
    
    let pwrcfg = dp.PWR.constrain().vos3().freeze();
    
    // Set all needed clock domains
    let _ccdr = dp
        .RCC
        .constrain()
        .use_hse(16.MHz())
        .sys_ck(200.MHz())
        .hclk(100.MHz())
        .pll1_q_ck(50.MHz()) // CAN
        .pll2_p_ck(100.MHz()) // ?
        .pll2_r_ck(50.MHz()) // LCD
        .freeze(pwrcfg, &dp.SYSCFG);
    
    // Enable cortex m7 cache and cyclecounter
    cp.SCB.enable_icache();
    cp.DWT.enable_cycle_counter();

    const STORAGE: usize = 0x0810_0000;
    let meta_data = unsafe { core::mem::transmute::<usize, &MetaData>(STORAGE) };
    let upper_flash_u32 =  unsafe { core::mem::transmute::<usize, &[u32; 0x2_0000]>(STORAGE) };
    let new_app_start_idx = meta_data.new_app - STORAGE;
    let new_app_end_idx = new_app_start_idx + meta_data.new_app_len;

    // Check magic number
    if meta_data.magic != 0x1c80_73ab_2085_3579 {
        loop {}; // We should never come here
    }

    // Check CRC of uploaded data
    let crc = stm32_crc(&upper_flash_u32[3..new_app_end_idx/4]);


    if crc != meta_data.crc {
        loop {}; // We should never come here;
    }

    // Note: You should also check whether the hardware version is correct and whether it makes 
    // sense to reflash the software (software version).

    // This call starts the update. First the consistency of the loaded data is checked, then the 
    // data from the upper flash area is copied to the lower one and then the new app is started.
    let func = unsafe { core::mem::transmute::<usize, fn()>(meta_data.copy_func) };
    func();

    loop {}; // We should never come here;
}

pub fn stm32_crc(data: &[u32]) -> u32 {
    let mut crc: u32 = 0xffffffff;
    for w in data {
        for val in w.to_be_bytes() {
            crc ^= (val as u32) << 24;
            for _ in 0..8 {
                if (crc & 0x8000_0000) == 0 {
                    crc <<= 1;
                } else {
                    crc = crc.wrapping_shl(1) ^ 0x04c1_1db7;
                }
            }
        }
    }
    crc
}


