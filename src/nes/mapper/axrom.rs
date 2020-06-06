use super::mapper::Mapper;
use crate::nes::cartridge::Cartridge;
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
pub struct AxROM {
    cartridge: Cartridge,
    prg_banks: u8,
    prg_bank1: u8,
}

impl AxROM {
    pub fn new(cartridge: Cartridge) -> Self {
        let banks = cartridge.header.prgsz;
        AxROM {
            cartridge: cartridge,
            prg_banks: banks,
            prg_bank1: 0,
        }
    }
}

#[typetag::serde]
impl Mapper for AxROM {
    fn borrow_cart(&self) -> &Cartridge {
        &self.cartridge
    }
    fn borrow_cart_mut(&mut self) -> &mut Cartridge {
        &mut self.cartridge
    }

    fn read(&mut self, address: u16) -> u8 {
        if address < 0x2000 {
            self.cartridge.chr[address as usize]
        } else if address >= 0x6000 && address < 0x8000 {
            self.cartridge.sram.read((address & 0x1FFF) as usize)
        } else if address >= 0x8000 {
            let a = ((self.prg_bank1 & 0xF) as usize) * 0x8000 + (address & 0x7FFF) as usize;
            self.cartridge.prg[a]
        } else {
            warn!("AxROM: unhandled read address={:x}", address);
            0
        }
    }

    fn write(&mut self, address: u16, value: u8) {
        if address < 0x2000 {
            self.cartridge.chr[address as usize] = value;
        } else if address >= 0x6000 && address < 0x8000 {
            self.cartridge
                .sram
                .write((address & 0x1FFF) as usize, value);
        } else if address >= 0x8000 {
            self.prg_bank1 = value % self.prg_banks;
        } else {
            warn!(
                "AxROM: unhandled write address={:x} value={:x}",
                address, value
            );
        }
    }
    fn mirror_address(&self, address: u16) -> u16 {
        // Currently unimplemented:
        // if (self.prg_bank1 & 0x10) { 1 KiB VRAM page for all 4 nametables }
        self.cartridge.mirror_address(address)
    }
}
