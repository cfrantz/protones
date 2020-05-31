pub mod mapper;
mod mmc1;
mod uxrom;
mod cnrom;
pub use mapper::Mapper;

use crate::nes::cartridge::Cartridge;
use super::mapper::uxrom::UxROM;
use super::mapper::cnrom::CNROM;
use super::mapper::mmc1::MMC1;

pub fn new(cartridge: Cartridge) -> Box<dyn Mapper> {
    match cartridge.header.mapper {
        0 | 2 => Box::new(UxROM::new(cartridge)),
        1 => Box::new(MMC1::new(cartridge)),
        3 => Box::new(CNROM::new(cartridge)),
        _ => {
            panic!("Mapper {} not supported!", cartridge.header.mapper);
        }
    }
}
