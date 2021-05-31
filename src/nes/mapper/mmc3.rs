use super::mapper::simple_mirror_address;
use super::mapper::Mapper;
use crate::nes::cartridge::Cartridge;
use crate::nes::nes::Nes;
use crate::nes::ppu::{MASK_SHOWBG, MASK_SHOWSPRITES};
use crate::nes::sram::SRam;
use log::warn;
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
pub struct MMC3 {
    #[serde(skip)]
    cartridge: Cartridge,

    irq_enable: bool,
    register: u8,
    reload: u8,
    counter: u8,
    prg_mode: u8,
    chr_mode: u8,
    mirror_mode: u8,
    registers: [u8; 8],
    prg_offset: [usize; 4],
    chr_offset: [usize; 8],
    sram: SRam,
}

impl MMC3 {
    pub fn new(cartridge: Cartridge) -> Self {
        let prg_offset = [
            MMC3::prg_bank_offset(&cartridge, 0x00),
            MMC3::prg_bank_offset(&cartridge, 0x01),
            MMC3::prg_bank_offset(&cartridge, 0xFE),
            MMC3::prg_bank_offset(&cartridge, 0xFF),
        ];
        MMC3 {
            cartridge: cartridge,

            irq_enable: false,
            register: 0,
            reload: 0,
            counter: 0,
            prg_mode: 0,
            chr_mode: 0,
            mirror_mode: 0,
            registers: [0u8; 8],
            prg_offset: prg_offset,
            chr_offset: [0; 8],
            sram: SRam::with_size(8192),
        }
    }

    fn prg_bank_offset(cartridge: &Cartridge, bank: u8) -> usize {
        let bank = bank % (cartridge.header.prgsz * 2);
        return 0x2000 * bank as usize;
    }
    fn chr_bank_offset(cartridge: &Cartridge, bank: u8) -> usize {
        let bank = bank % (cartridge.header.chrsz * 8);
        return 0x0400 * bank as usize;
    }
    fn update_offsets(&mut self) {
        self.prg_offset = match self.prg_mode {
            0 => [
                MMC3::prg_bank_offset(&self.cartridge, self.registers[6]),
                MMC3::prg_bank_offset(&self.cartridge, self.registers[7]),
                MMC3::prg_bank_offset(&self.cartridge, 0xFE),
                MMC3::prg_bank_offset(&self.cartridge, 0xFF),
            ],
            1 => [
                MMC3::prg_bank_offset(&self.cartridge, 0xFE),
                MMC3::prg_bank_offset(&self.cartridge, self.registers[7]),
                MMC3::prg_bank_offset(&self.cartridge, self.registers[6]),
                MMC3::prg_bank_offset(&self.cartridge, 0xFF),
            ],
            _ => {
                panic!("Bad prg_mode {}", self.prg_mode);
            }
        };
        self.chr_offset = match self.chr_mode {
            0 => [
                MMC3::chr_bank_offset(&self.cartridge, self.registers[0] & 0xFE),
                MMC3::chr_bank_offset(&self.cartridge, self.registers[0] | 0x01),
                MMC3::chr_bank_offset(&self.cartridge, self.registers[1] & 0xFE),
                MMC3::chr_bank_offset(&self.cartridge, self.registers[1] | 0x01),
                MMC3::chr_bank_offset(&self.cartridge, self.registers[2]),
                MMC3::chr_bank_offset(&self.cartridge, self.registers[3]),
                MMC3::chr_bank_offset(&self.cartridge, self.registers[4]),
                MMC3::chr_bank_offset(&self.cartridge, self.registers[5]),
            ],
            1 => [
                MMC3::chr_bank_offset(&self.cartridge, self.registers[2]),
                MMC3::chr_bank_offset(&self.cartridge, self.registers[3]),
                MMC3::chr_bank_offset(&self.cartridge, self.registers[4]),
                MMC3::chr_bank_offset(&self.cartridge, self.registers[5]),
                MMC3::chr_bank_offset(&self.cartridge, self.registers[0] & 0xFE),
                MMC3::chr_bank_offset(&self.cartridge, self.registers[0] | 0x01),
                MMC3::chr_bank_offset(&self.cartridge, self.registers[1] & 0xFE),
                MMC3::chr_bank_offset(&self.cartridge, self.registers[1] | 0x01),
            ],
            _ => {
                panic!("Bad chr_mode {}", self.chr_mode);
            }
        };
    }
    fn write_bank_select(&mut self, val: u8) {
        self.prg_mode = (val >> 6) & 1;
        self.chr_mode = (val >> 7) & 1;
        self.register = val & 7;
    }

    fn write_mirror_mode(&mut self, val: u8) {
        self.mirror_mode = 1 - (val & 1);
    }

    fn write_register(&mut self, addr: u16, val: u8) {
        if addr < 0xA000 {
            if (addr & 1) == 0 {
                self.write_bank_select(val);
            } else {
                self.registers[self.register as usize] = val;
            }
            self.update_offsets();
        } else if addr < 0xC000 {
            if (addr & 1) == 0 {
                self.write_mirror_mode(val);
            }
        } else if addr < 0xE000 {
            if (addr & 1) == 0 {
                self.reload = val;
            } else {
                self.counter = 0;
            }
        } else {
            self.irq_enable = (addr & 1) != 0;
        }
    }
}

#[typetag::serde]
impl Mapper for MMC3 {
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
            let bank = (address / 0x0400) as usize;
            let offset = (address % 0x0400) as usize;
            self.cartridge.chr[self.chr_offset[bank] + offset]
        } else if address >= 0x6000 && address < 0x8000 {
            self.sram.read((address & 0x1FFF) as usize)
        } else if address >= 0x8000 {
            let address = (address & 0x7FFF) as usize;
            let bank = address / 0x2000;
            let offset = address % 0x2000;
            self.cartridge.prg[self.prg_offset[bank] + offset]
        } else {
            warn!("MMC3: unhandled read address={:x}", address);
            0
        }
    }

    fn write(&mut self, address: u16, value: u8) {
        if address < 0x2000 {
            let bank = (address / 0x0400) as usize;
            let offset = (address % 0x0400) as usize;
            self.cartridge.chr[self.chr_offset[bank] + offset] = value;
        } else if address >= 0x6000 && address < 0x8000 {
            self.sram.write((address & 0x1FFF) as usize, value);
        } else if address >= 0x8000 {
            self.write_register(address, value);
        } else {
            warn!(
                "MMC3: unhandled write address={:x} value={:x}",
                address, value
            );
        }
    }

    fn emulate(&mut self, nes: &Nes) {
        let ppu = nes.ppu.borrow();
        if ppu.cycle == 280
            && (ppu.scanline < 240 || ppu.scanline == 260)
            && (ppu.mask & (MASK_SHOWBG | MASK_SHOWSPRITES)) != 0
        {
            if self.counter == 0 {
                self.counter = self.reload;
            } else {
                self.counter -= 1;
                if self.counter == 0 && self.irq_enable {
                    nes.signal_irq();
                }
            }
        }
    }

    fn mirror_address(&self, address: u16) -> u16 {
        let mode = if self.cartridge.header.fourscreen {
            4
        } else {
            self.mirror_mode
        };
        simple_mirror_address(mode, address)
    }
}
