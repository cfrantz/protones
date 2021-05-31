use crate::nes::cartridge::Cartridge;
use crate::nes::nes::Nes;
use crate::nes::sram::SRam;
use std::io;
use std::path::PathBuf;

pub fn simple_mirror_address(mode: u8, address: u16) -> u16 {
    let address = address & 0xFFF;
    match mode {
        0 => {
            let a11 = address & 0x800;
            (address & !0xC00) | (a11 >> 1)
        }
        1 => address & !0x800,
        2 => address & !0xC00,
        3 => (address & !0xC00) | 0x400,
        4 => address,
        _ => {
            panic!("Unknown mirror mode {}", mode);
        }
    }
}

#[typetag::serde(tag = "type")]
pub trait Mapper {
    fn borrow_cart(&self) -> &Cartridge;
    fn set_cartridge(&mut self, cart: Cartridge);

    fn borrow_sram(&self) -> &SRam;
    fn borrow_sram_mut(&mut self) -> &mut SRam;

    fn read(&mut self, address: u16) -> u8;
    fn write(&mut self, address: u16, value: u8);
    fn emulate(&mut self, _nes: &Nes) {}
    fn mirror_address(&self, address: u16) -> u16;

    fn sram_load(&mut self, filepath: &PathBuf) -> io::Result<()> {
        let battery = self.borrow_cart().header.battery;
        if battery {
            let sram = self.borrow_sram_mut();
            sram.load(filepath)
        } else {
            Ok(())
        }
    }

    fn sram_save(&self, filepath: &PathBuf) -> io::Result<()> {
        let battery = self.borrow_cart().header.battery;
        if battery {
            let sram = self.borrow_sram();
            sram.save(filepath)
        } else {
            Ok(())
        }
    }

    fn expansion_audio(&mut self, _nes: &Nes) -> f32 {
        0.0f32
    }
}
