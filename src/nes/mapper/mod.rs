mod cnrom;
pub mod mapper;
mod mmc1;
mod mmc3;
mod uxrom;
pub use mapper::Mapper;

use super::mapper::cnrom::CNROM;
use super::mapper::mmc1::MMC1;
use super::mapper::mmc3::MMC3;
use super::mapper::uxrom::UxROM;
use crate::nes::cartridge::Cartridge;

pub fn new(cartridge: Cartridge) -> Box<dyn Mapper> {
    match cartridge.header.mapper {
        0 | 2 => Box::new(UxROM::new(cartridge)),
        1 => Box::new(MMC1::new(cartridge)),
        3 => Box::new(CNROM::new(cartridge)),
        4 => Box::new(MMC3::new(cartridge)),
        _ => {
            panic!("Mapper {} not supported!", cartridge.header.mapper);
        }
    }
}
