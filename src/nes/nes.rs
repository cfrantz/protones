use crate::nes::apu::Apu;
use crate::nes::cartridge::Cartridge;
use crate::nes::controller::Controller;
use crate::nes::cpu6502::{Cpu6502, Memory};
use crate::nes::mapper;
use crate::nes::mapper::Mapper;
use crate::nes::ppu::Ppu;
use log::{trace, warn};
use serde::{Deserialize, Serialize};
use std::cell::RefCell;
use std::io;

const PALETTE: [u32; 64] = [
    // ABGR
    0xff666666, 0xff882a00, 0xffa71214, 0xffa4003b, 0xff7e005c, 0xff40006e, 0xff00066c, 0xff001d56,
    0xff003533, 0xff00480b, 0xff005200, 0xff084f00, 0xff4d4000, 0xff000000, 0xff000000, 0xff000000,
    0xffadadad, 0xffd95f15, 0xffff4042, 0xfffe2775, 0xffcc1aa0, 0xff7b1eb7, 0xff2031b5, 0xff004e99,
    0xff006d6b, 0xff008738, 0xff00930c, 0xff328f00, 0xff8d7c00, 0xff000000, 0xff000000, 0xff000000,
    0xfffffeff, 0xffffb064, 0xffff9092, 0xffff76c6, 0xffff6af3, 0xffcc6efe, 0xff7081fe, 0xff229eea,
    0xff00bebc, 0xff00d888, 0xff30e45c, 0xff82e045, 0xffdecd48, 0xff4f4f4f, 0xff000000, 0xff000000,
    0xfffffeff, 0xffffdfc0, 0xffffd2d3, 0xffffc8e8, 0xffffc2fb, 0xffeac4fe, 0xffc5ccfe, 0xffa5d8f7,
    0xff94e5e4, 0xff96efcf, 0xffabf4bd, 0xffccf3b3, 0xfff2ebb5, 0xffb8b8b8, 0xff000000, 0xff000000,
];

#[derive(Debug, Serialize, Deserialize)]
struct Stall {
    cycles: u32,
    oddcycle: bool,
}

#[derive(Serialize, Deserialize)]
pub struct Nes {
    pub mapper: RefCell<Box<dyn Mapper>>,
    pub cpu: RefCell<Cpu6502>,
    pub ppu: RefCell<Ppu>,
    pub apu: RefCell<Apu>,
    pub controller: Vec<RefCell<Controller>>,

    pub ram: RefCell<Vec<u8>>,
    pub vram: RefCell<Vec<u8>>,
    pub pram: RefCell<Vec<u8>>,
    pub palette: RefCell<Vec<u32>>,
    remainder: f64,
    pub frame: u64,
    pub trace: bool,
    pub audio: RefCell<Vec<f32>>,
    stall: RefCell<Stall>,
    pub pause: bool,
    pub framestep: bool,
}

impl Memory for Nes {
    fn read(&self, address: u16) -> u8 {
        if address < 0x2000 {
            self.ram.borrow()[(address & 0x7FF) as usize]
        } else if address < 0x4000 || address == 0x4014 {
            self.ppu.borrow_mut().read(self, address)
        } else if address == 0x4015 {
            self.apu.borrow_mut().read(address)
        } else if address == 0x4016 {
            self.controller[0].borrow_mut().read()
        } else if address == 0x4017 {
            self.controller[1].borrow_mut().read()
        } else if address >= 0x5000 {
            self.mapper.borrow_mut().read(address)
        } else {
            warn!("Unhandled memory read from {:x}", address);
            0
        }
    }

    fn write(&self, address: u16, value: u8) {
        if address < 0x2000 {
            self.ram.borrow_mut()[(address & 0x7FF) as usize] = value;
        } else if address < 0x4000 || address == 0x4014 {
            self.ppu.borrow_mut().write(self, address, value);
        } else if address == 0x4016 {
            self.controller[0].borrow_mut().write(value);
            self.controller[1].borrow_mut().write(value);
        } else if address >= 0x4000 && address <= 0x4017 {
            self.apu.borrow_mut().write(address, value);
        } else if address >= 0x5000 {
            self.mapper.borrow_mut().write(address, value)
        } else {
            warn!("Unhandled memory write to {:x}", address);
        }
    }
}

impl Nes {
    pub const FREQUENCY: usize = 1789773;
    pub const FRAME_COUNTER_RATE: f64 = (Nes::FREQUENCY as f64) / 240.0;
    pub const SAMPLE_RATE: f64 = (Nes::FREQUENCY as f64) / 48000.0;
    pub const FPS: f64 = 60.0998;

    pub fn from_file(filename: &str) -> io::Result<Self> {
        let cartridge = Cartridge::from_file(filename)?;
        let mapper = mapper::new(cartridge);
        let mut palette = Vec::<u32>::new();
        palette.extend_from_slice(&PALETTE);

        Ok(Nes {
            mapper: RefCell::new(mapper),
            cpu: RefCell::new(Cpu6502::default()),
            ppu: RefCell::new(Ppu::new()),
            apu: RefCell::new(Apu::new()),
            controller: vec![
                RefCell::new(Controller::default()),
                RefCell::new(Controller::default()),
            ],
            ram: RefCell::new(vec![0u8; 2048]),
            vram: RefCell::new(vec![0u8; 4096]),
            pram: RefCell::new(vec![0u8; 32]),
            palette: RefCell::new(palette),
            remainder: 0f64,
            frame: 0,
            trace: false,
            audio: RefCell::new(Vec::<f32>::new()),
            stall: RefCell::new(Stall {
                cycles: 0,
                oddcycle: false,
            }),
            pause: false,
            framestep: false,
        })
    }

    pub fn reset(&self) {
        self.cpu.borrow_mut().reset();
        self.ppu.borrow_mut().reset();
    }

    pub fn signal_nmi(&self) {
        self.cpu.borrow_mut().signal_nmi();
    }
    pub fn signal_irq(&self) {
        self.cpu.borrow_mut().signal_irq();
    }
    pub fn palette_lookup(&self, color: u8) -> u32 {
        let pcolor = self.pram.borrow()[color as usize];
        self.palette.borrow()[pcolor as usize]
    }
    fn palette_addr(address: u16) -> usize {
        let address = address & 0x1F;
        let a = if address >= 16 && (address % 4) == 0 {
            address - 16
        } else {
            address
        };
        a as usize
    }

    pub fn ppu_read(&self, address: u16) -> u8 {
        if address < 0x2000 {
            self.mapper.borrow_mut().read(address)
        } else if address < 0x3f00 {
            let address = self.mapper.borrow().mirror_address(address);
            self.vram.borrow()[address as usize]
        } else if address < 0x4000 {
            self.pram.borrow()[(address & 0x1F) as usize]
        } else {
            warn!("Unhandled ppu read from {:x}", address);
            0
        }
    }
    pub fn ppu_write(&self, address: u16, val: u8) {
        if address < 0x2000 {
            self.mapper.borrow_mut().write(address, val)
        } else if address < 0x3f00 {
            let address = self.mapper.borrow().mirror_address(address);
            self.vram.borrow_mut()[address as usize] = val;
        } else if address < 0x4000 {
            self.pram.borrow_mut()[Nes::palette_addr(address)] = val;
        } else {
            warn!("Unhandled ppu write to {:x}", address);
        }
    }

    pub fn add_stall(&self, cycles: u32, oddcycle: bool) {
        let mut stall = self.stall.borrow_mut();
        stall.cycles += cycles;
        if oddcycle {
            stall.oddcycle = true;
        }
    }
    pub fn clear_stall(&self) {
        let mut stall = self.stall.borrow_mut();
        stall.cycles = 0;
        stall.oddcycle = false;
    }

    pub fn emulate(&self) -> bool {
        if self.trace {
            let cpu = self.cpu.borrow();
            let (instr, _) = Cpu6502::disassemble(self, cpu.pc);
            println!("          {}", cpu.cpustate());
            println!("{}:     {}", cpu.get_cycles(), instr);
        }
        let cycle = self.cpu.borrow().get_cycles();
        let mut n = self.cpu.borrow_mut().execute(self);
        if self.stall.borrow().cycles != 0 {
            n += self.stall.borrow().cycles;
            if self.stall.borrow().oddcycle && cycle % 2 != 0 {
                n += 1;
            }
            self.clear_stall();
        }
        for _ in 0..n {
            if let Some(sample) = self.apu.borrow_mut().emulate(self) {
                self.audio.borrow_mut().push(sample);
            }
            for _ in 0..3 {
                // The PPU is clocked at 3x the rate of the cpu
                self.ppu.borrow_mut().emulate(self);
                self.mapper.borrow_mut().emulate(self);
            }
        }
        n != 0
    }

    pub fn emulate_frame(&mut self) {
        if self.pause {
            if self.framestep {
                self.framestep = false;
            } else {
                return;
            }
        }
        //let count = (Nes::FREQUENCY as f64) / Nes::FPS - self.remainder;
        //let cycle = self.cpu.borrow().get_cycles();
        //let eof = count + cycle as f64;
        //while (self.cpu.borrow().get_cycles() as f64) < eof {
        //    if !self.emulate() {
        //        break;
        //    }
        //}

        let cycle = self.cpu.borrow().get_cycles();
        let frame = self.ppu.borrow().frame;
        while self.ppu.borrow().frame == frame {
            if !self.emulate() {
                break;
            }
        }
        let count = self.cpu.borrow().get_cycles() - cycle;
        self.frame += 1;
        //self.remainder = self.cpu.borrow().get_cycles() as f64 - eof;
        trace!(
            "frame={} cycle={} count={:.2} remainder={:.2}",
            self.frame,
            cycle,
            count,
            self.remainder
        );
    }
}
