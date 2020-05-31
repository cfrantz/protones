use super::mapper::simple_mirror_address;
use super::mapper::Mapper;
use crate::nes::cartridge::Cartridge;
use log::warn;

#[derive(Clone, Debug, Default)]
pub struct MMC1 {
    cartridge: Cartridge,
    shift_register: u8,
    control: u8,
    prg_mode: u8,
    chr_mode: u8,
    prg_bank: u8,
    chr_bank: [u8; 2],
    prg_offset: [usize; 2],
    chr_offset: [usize; 2],
}

impl MMC1 {
    pub fn new(cartridge: Cartridge) -> Self {
        let prg_offset = MMC1::prg_bank_offset(&cartridge, 0xFF);
        MMC1 {
            cartridge: cartridge,
            shift_register: 0x10,
            prg_offset: [0, prg_offset],
            ..Default::default()
        }
    }

    fn prg_bank_offset(cartridge: &Cartridge, bank: u8) -> usize {
        let bank = bank % cartridge.header.prgsz;
        return 0x4000 * bank as usize;
    }
    fn chr_bank_offset(cartridge: &Cartridge, bank: u8) -> usize {
        let bank = bank % (cartridge.header.chrsz * 2);
        return 0x1000 * bank as usize;
    }
    fn load_register(&mut self, addr: u16, val: u8) {
        if val & 0x80 != 0 {
            self.shift_register = 0x10;
            self.write_control(self.control | 0x0C);
            self.update_offsets();
        } else {
            let complete = (self.shift_register & 1) != 0;
            self.shift_register = (self.shift_register >> 1) | ((val & 1) << 4);
            if complete {
                self.write_register(addr, self.shift_register);
                self.shift_register = 0x10;
            }
        }
    }
    fn write_register(&mut self, addr: u16, val: u8) {
        if addr < 0xA000 {
            self.write_control(val);
        } else if addr < 0xC000 {
            self.chr_bank[0] = val;
        } else if addr < 0xE000 {
            self.chr_bank[1] = val;
        } else {
            self.prg_bank = val & 0x0F;
        }
        self.update_offsets();
    }
    fn update_offsets(&mut self) {
        self.prg_offset = match self.prg_mode {
            0 | 1 => [
                MMC1::prg_bank_offset(&self.cartridge, self.prg_bank & 0xFE),
                MMC1::prg_bank_offset(&self.cartridge, self.prg_bank | 0x01),
            ],
            2 => [0, MMC1::prg_bank_offset(&self.cartridge, self.prg_bank)],
            3 => [
                MMC1::prg_bank_offset(&self.cartridge, self.prg_bank),
                MMC1::prg_bank_offset(&self.cartridge, 0xFF),
            ],
            _ => {
                panic!("Bad prg_mode {}", self.prg_mode);
            }
        };
        self.chr_offset = match self.chr_mode {
            0 => [
                MMC1::chr_bank_offset(&self.cartridge, self.chr_bank[0] & 0xFE),
                MMC1::chr_bank_offset(&self.cartridge, self.chr_bank[0] | 0x01),
            ],
            1 => [
                MMC1::chr_bank_offset(&self.cartridge, self.chr_bank[0]),
                MMC1::chr_bank_offset(&self.cartridge, self.chr_bank[1]),
            ],
            _ => {
                panic!("Bad chr_mode {}", self.chr_mode);
            }
        };
    }
    fn write_control(&mut self, val: u8) {
        self.control = val;
        self.chr_mode = (val >> 4) & 1;
        self.prg_mode = (val >> 2) & 3;
    }
}

impl Mapper for MMC1 {
    fn read(&mut self, address: u16) -> u8 {
        if address < 0x2000 {
            let bank = (address / 0x1000) as usize;
            let offset = (address % 0x1000) as usize;
            self.cartridge.chr[self.chr_offset[bank] + offset]
        } else if address >= 0x6000 && address < 0x8000 {
            self.cartridge.sram[(address & 0x1FFF) as usize]
        } else if address >= 0x8000 {
            let address = (address & 0x7FFF) as usize;
            let bank = address / 0x4000;
            let offset = address % 0x4000;
            self.cartridge.prg[self.prg_offset[bank] + offset]
        } else {
            warn!("MMC1: unhandled read address={:x}", address);
            0
        }
    }

    fn write(&mut self, address: u16, value: u8) {
        if address < 0x2000 {
            let bank = (address / 0x1000) as usize;
            let offset = (address % 0x1000) as usize;
            self.cartridge.chr[self.chr_offset[bank] + offset] = value;
        } else if address >= 0x6000 && address < 0x8000 {
            self.cartridge.sram[(address & 0x1FFF) as usize] = value;
        } else if address >= 0x8000 {
            self.load_register(address, value);
        } else {
            warn!(
                "MMC1: unhandled write address={:x} value={:x}",
                address, value
            );
        }
    }

    fn mirror_address(&self, address: u16) -> u16 {
        let mode = if self.cartridge.header.fourscreen {
            4
        } else {
            !self.control & 3
        };
        simple_mirror_address(mode, address)
    }
}
