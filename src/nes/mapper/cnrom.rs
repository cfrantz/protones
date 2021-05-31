use super::mapper::Mapper;
use crate::nes::cartridge::Cartridge;
use crate::nes::sram::SRam;
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
pub struct CNROM {
    #[serde(skip)]
    cartridge: Cartridge,
    chr_banks: u8,
    chr_bank1: u8,
    sram: SRam,
}

impl CNROM {
    pub fn new(cartridge: Cartridge) -> Self {
        let chrsz = cartridge.header.chrsz;
        CNROM {
            cartridge: cartridge,
            chr_banks: chrsz,
            chr_bank1: 0,
            sram: SRam::with_size(8192),
        }
    }
}

#[typetag::serde]
impl Mapper for CNROM {
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
            let a = (self.chr_bank1 as usize) * 0x2000 + address as usize;
            self.cartridge.chr[a]
        } else if address >= 0x6000 && address < 0x8000 {
            self.sram.read((address & 0x1FFF) as usize)
        } else if address >= 0x8000 {
            self.cartridge.prg[(address & 0x7FFF) as usize]
        } else {
            warn!("CNROM: unhandled read address={:x}", address);
            0
        }
    }

    fn write(&mut self, address: u16, value: u8) {
        if address < 0x2000 {
            self.cartridge.chr[address as usize] = value;
        } else if address >= 0x6000 && address < 0x8000 {
            self.sram.write((address & 0x1FFF) as usize, value);
        } else if address >= 0x8000 {
            self.chr_bank1 = value & 3;
        } else {
            warn!(
                "CNROM: unhandled write address={:x} value={:x}",
                address, value
            );
        }
    }
    fn mirror_address(&self, address: u16) -> u16 {
        self.cartridge.mirror_address(address)
    }
}
