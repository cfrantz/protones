use super::mapper::Mapper;
use crate::nes::cartridge::Cartridge;

#[derive(Clone, Debug, Default)]
pub struct UxROM {
    cartridge: Cartridge,
    prg_banks: u8,
    prg_bank1: u8,
    prg_bank2: u8,
}

impl UxROM {
    pub fn new(cartridge: Cartridge) -> Self {
        let banks = cartridge.header.prgsz;
        UxROM {
            cartridge: cartridge,
            prg_banks: banks,
            prg_bank1: 0,
            prg_bank2: banks - 1,
        }
    }
}

impl Mapper for UxROM {
    fn read(&mut self, address: u16) -> u8 {
        if address < 0x2000 {
            self.cartridge.chr[address as usize]
        } else if address >= 0x6000 && address < 0x8000 {
            self.cartridge.sram[(address & 0x1FFF) as usize]
        } else if address < 0xC000 {
            let a = (self.prg_bank1 as usize) * 0x4000 +
                    (address & 0x3FFF) as usize;
            self.cartridge.prg[a]
        } else if address >= 0xC000 {
            let a = (self.prg_bank2 as usize) * 0x4000 +
                    (address & 0x3FFF) as usize;
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
            self.cartridge.sram[(address & 0x1FFF) as usize] = value;
        } else if address >= 0x8000 {
            self.prg_bank1 = value % self.prg_banks;
        } else {
            warn!("UxROM: unhandled write address={:x} value={:x}",
                  address, value);
        }
    }
    fn mirror_address(&self, address: u16) -> u16 {
        self.cartridge.mirror_address(address)
    }
}
