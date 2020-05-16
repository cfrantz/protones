use std::cell::RefCell;
use crate::nes::mapper::Mapper;
use crate::nes::cpu6502::{Cpu6502, Memory};
use crate::nes::ppu::Ppu;
use crate::nes::apu::Apu;
use crate::nes::controller::Controller;


pub struct Nes {
    pub mapper: RefCell<Box<dyn Mapper>>,
    pub cpu: RefCell<Cpu6502>,
    pub ppu: RefCell<Ppu>,
    pub apu: RefCell<Apu>,
    pub controller: Vec<RefCell<Controller>>,

    ram: Vec<u8>,
    vram: Vec<u8>,
    palette: Vec<u32>,
}

impl Memory for Nes {
    fn read(&mut self, address: u16) -> u8 {
        if address < 0x2000 {
            self.ram[(address & 0x7FF) as usize]
        } else if address < 0x4000 || address == 0x4014 {
            let mut ppu = self.ppu.borrow_mut();
            ppu.read(self, address)
        } else if address == 0x4015 {
            self.apu.borrow_mut().read(address)
        } else if address == 0x4016 {
            self.controller[0].borrow_mut().read()
        } else if address == 0x4017 {
            self.controller[1].borrow_mut().read()
        } else if address >= 0x5000 {
            self.mapper.borrow_mut().read(address)
        } else {
            0
        }
    }

    fn write(&mut self, address: u16, value: u8) {
        if address < 0x2000 {
            self.ram[(address & 0x7FF) as usize] = value;
        } else if address < 0x4000 || address == 0x4014 {
            let mut ppu = self.ppu.borrow_mut();
            ppu.write(self, address, value);
        } else if address == 0x4016 {
            self.controller[0].borrow_mut().write(value);
            self.controller[1].borrow_mut().write(value);
        } else if address >= 0x4000 && address <= 0x4017 {
            self.apu.borrow_mut().write(address, value);
        } else if address >= 0x5000 {
            self.mapper.borrow_mut().write(address, value)
        } else {
            // print an error message 
        }
    }
}

impl Nes {
    pub const FREQUENCY: usize = 1789773;
    pub const FRAME_COUNTER_RATE: f64 = (Nes::FREQUENCY as f64) / 240.0;
    pub const SAMPLE_RATE: f64 = (Nes::FREQUENCY as f64) / 44100.0;

    pub fn signal_nmi(&self) {
        self.cpu.borrow_mut().signal_nmi();
    }
    pub fn signal_irq(&self) {
        self.cpu.borrow_mut().signal_irq();
    }
    pub fn palette_lookup(&self, color: u8) -> u32 {
        self.palette[color as usize]
    }
    pub fn ppu_read(&mut self, address: u16) -> u8 {
        if address < 0x2000 {
            0
        } else if address < 0x3f00 {
            self.vram[(address & 0xFFF) as usize]
        } else {
            0
        }
    }
    pub fn ppu_write(&mut self, address: u16, val: u8) {
        if address < 0x2000 {
        } else if address < 0x3f00 {
            self.vram[(address & 0xFFF) as usize] = val;
        } else {
        }
    }
}
