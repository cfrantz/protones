use super::mapper::Mapper;
use crate::nes::cartridge::Cartridge;
use crate::nes::sram::SRam;
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
pub struct UxROM {
    #[serde(skip)]
    cartridge: Cartridge,
    prg_banks: u8,
    prg_bank1: u8,
    prg_bank2: u8,
    sram: SRam,
}

impl UxROM {
    pub fn new(cartridge: Cartridge) -> Self {
        let banks = cartridge.header.prgsz;
        UxROM {
            cartridge: cartridge,
            prg_banks: banks,
            prg_bank1: 0,
            prg_bank2: banks - 1,
            sram: SRam::with_size(8192),
        }
    }
}

#[typetag::serde]
impl Mapper for UxROM {
    fn borrow_cart(&self) -> &Cartridge {
        &self.cartridge
    }
    fn set_cartridge(&mut self, cart: Cartridge) {
        self.cartridge = cart;
    }
    fn borrow_sram(&self) -> &SRam {
        &self.sram
    }
    fn borrow_sram_mut(&mut self) -> &mut SRam {
        &mut self.sram
    }

    fn read(&mut self, address: u16) -> u8 {
        if address < 0x2000 {
            self.cartridge.chr[address as usize]
        } else if address >= 0x6000 && address < 0x8000 {
            self.sram.read((address & 0x1FFF) as usize)
        } else if address < 0xC000 {
            let a = (self.prg_bank1 as usize) * 0x4000 + (address & 0x3FFF) as usize;
            self.cartridge.prg[a]
        } else if address >= 0xC000 {
            let a = (self.prg_bank2 as usize) * 0x4000 + (address & 0x3FFF) as usize;
            self.cartridge.prg[a]
        } else {
            warn!("UxROM: unhandled read address={:x}", address);
            0
        }
    }

    fn write(&mut self, address: u16, value: u8) {
        if address < 0x2000 {
            self.cartridge.chr[address as usize] = value;
        } else if address >= 0x6000 && address < 0x8000 {
            self.sram.write((address & 0x1FFF) as usize, value);
        } else if address >= 0x8000 {
            self.prg_bank1 = value % self.prg_banks;
        } else {
            warn!(
                "UxROM: unhandled write address={:x} value={:x}",
                address, value
            );
        }
    }
    fn mirror_address(&self, address: u16) -> u16 {
        self.cartridge.mirror_address(address)
    }
}
