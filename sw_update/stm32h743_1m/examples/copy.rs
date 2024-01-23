#![no_main]
#![no_std]

use cortex_m_rt::entry;
//use stm32h7xx_hal::{pac, prelude::*};


use stm32h7xx_hal::{
    pac::Peripherals,
    flash::FlashExt,
};
use embedded_storage::nor_flash::NorFlash;

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    cortex_m::peripheral::SCB::sys_reset();
}

include!("../../common/meta_v1.rs");

// We simulate the loading of the image by flashing it with the debugger to the upper area.
//
// $ mon program image.bin 0x08100000
//
// Then start test app
#[entry]
fn main() -> ! {
    /*let mut cp = cortex_m::Peripherals::take().unwrap();
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
    cp.DWT.enable_cycle_counter();*/

    copy_image();

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

fn copy_image() -> !  {
    cortex_m::interrupt::disable();

    let dp = unsafe { Peripherals::steal() };

    const STORAGE: usize = 0x0810_0000;
    let meta_data = unsafe { core::mem::transmute::<usize, &MetaData>(STORAGE) };
    let upper_flash_u32 =  unsafe { core::mem::transmute::<usize, &[u32; 0x2_0000]>(STORAGE) };
    let upper_flash_u8 =  unsafe { core::mem::transmute::<usize, &[u8; 0x8_0000]>(STORAGE) };
    let new_app_start_idx = meta_data.new_app - STORAGE;
    let new_app_end_idx = new_app_start_idx + meta_data.new_app_len;

    if meta_data.magic != 0x1c80_73ab_2085_3579 {
        cortex_m::peripheral::SCB::sys_reset();
    }

    let crc = stm32_crc(&upper_flash_u32[3..new_app_end_idx/4]);

    if crc != meta_data.crc {
        cortex_m::peripheral::SCB::sys_reset();
    }

    let (mut flash, _) = dp.FLASH.split();
    let mut unlocked_flash = flash.unlocked();
    NorFlash::erase(&mut unlocked_flash, 0, meta_data.new_app_len as u32).unwrap();
    NorFlash::write(&mut unlocked_flash, 0, &upper_flash_u8[new_app_start_idx..new_app_end_idx]).unwrap();

    // Lock flash again
    drop(unlocked_flash);

    cortex_m::peripheral::SCB::sys_reset()
}

