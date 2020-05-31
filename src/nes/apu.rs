use crate::nes::apu_dmc::ApuDmc;
use crate::nes::apu_noise::ApuNoise;
use crate::nes::apu_pulse::ApuPulse;
use crate::nes::apu_triangle::ApuTriangle;
use crate::nes::nes::Nes;
use std::default::Default;

#[derive(Clone, Debug, Default)]
pub struct Apu {
    cycle: u64,
    frame_period: u8,
    frame_value: u8,
    frame_irq: bool,
    pub volume: f32,

    pub pulse0: ApuPulse,
    pub pulse1: ApuPulse,
    pub triangle: ApuTriangle,
    pub noise: ApuNoise,
    pub dmc: ApuDmc,
}

impl Apu {
    pub fn new() -> Self {
        Apu {
            volume: 1.0,
            pulse0: ApuPulse::new(0, 0.25),
            pulse1: ApuPulse::new(1, 0.25),
            triangle: ApuTriangle::new(0.25),
            noise: ApuNoise::new(0.25),
            dmc: ApuDmc::new(0.25),
            ..Default::default()
        }
    }
    fn step_timer(&mut self, nes: &Nes) {
        if self.cycle % 2 == 0 {
            self.pulse0.step_timer();
            self.pulse1.step_timer();
            self.noise.step_timer();
            self.dmc.step_timer(nes);
        }
        self.triangle.step_timer();
    }
    fn step_envelope(&mut self) {
        self.pulse0.step_envelope();
        self.pulse1.step_envelope();
        self.triangle.step_counter();
        self.noise.step_envelope();
    }
    fn step_sweep(&mut self) {
        self.pulse0.step_sweep();
        self.pulse1.step_sweep();
    }
    fn step_length(&mut self) {
        self.pulse0.step_length();
        self.pulse1.step_length();
        self.triangle.step_length();
        self.noise.step_length();
    }
    fn step_frame_counter(&mut self, nes: &Nes) {
        if self.frame_period == 4 {
            self.frame_value = (self.frame_value + 1) % 4;
            self.step_envelope();
            if (self.frame_value & 1) == 1 {
                self.step_sweep();
                self.step_length();
                if self.frame_value == 3 && self.frame_irq {
                    nes.signal_irq();
                }
            }
        } else {
            self.frame_value = (self.frame_value + 1) % 5;
            if self.frame_value != 4 {
                self.step_envelope();
                if (self.frame_value & 1) == 0 {
                    self.step_sweep();
                    self.step_length();
                }
            }
        }
    }
    fn output(&mut self) -> f32 {
        self.volume
            * (self.pulse0.output()
                + self.pulse1.output()
                + self.triangle.output()
                + self.noise.output()
                + self.dmc.output())
    }
    pub fn emulate(&mut self, nes: &Nes) -> Option<f32> {
        let c1 = self.cycle as f64;
        self.cycle += 1;
        let c2 = self.cycle as f64;

        self.step_timer(nes);

        let f1 = (c1 / Nes::FRAME_COUNTER_RATE) as usize;
        let f2 = (c2 / Nes::FRAME_COUNTER_RATE) as usize;
        if f1 != f2 {
            self.step_frame_counter(nes);
        }

        let s1 = (c1 / Nes::SAMPLE_RATE) as usize;
        let s2 = (c2 / Nes::SAMPLE_RATE) as usize;
        if s1 != s2 {
            Some(self.output())
        } else {
            None
        }
    }

    pub fn write(&mut self, address: u16, val: u8) {
        match address {
            0x4000 => self.pulse0.set_control(val),
            0x4001 => self.pulse0.set_sweep(val),
            0x4002 => self.pulse0.set_timer_low(val),
            0x4003 => self.pulse0.set_timer_high(val),

            0x4004 => self.pulse1.set_control(val),
            0x4005 => self.pulse1.set_sweep(val),
            0x4006 => self.pulse1.set_timer_low(val),
            0x4007 => self.pulse1.set_timer_high(val),

            0x4008 => self.triangle.set_control(val),
            0x400a => self.triangle.set_timer_low(val),
            0x400b => self.triangle.set_timer_high(val),

            0x400c => self.noise.set_control(val),
            0x400e => self.noise.set_period(val),
            0x400f => self.noise.set_length(val),

            0x4010 => self.dmc.set_control(val),
            0x4011 => self.dmc.set_value(val),
            0x4012 => self.dmc.set_address(val),
            0x4013 => self.dmc.set_length(val),

            0x4015 => self.set_control(val),
            0x4017 => self.set_frame_counter(val),

            _ => {}
        }
    }

    pub fn read(&mut self, address: u16) -> u8 {
        match address {
            0x4015 => {
                0x00 | if self.pulse0.active() { 0x01 } else { 0x00 }
                    | if self.pulse1.active() { 0x02 } else { 0x00 }
                    | if self.triangle.active() { 0x04 } else { 0x00 }
                    | if self.noise.active() { 0x08 } else { 0x00 }
                    | if self.dmc.active() { 0x10 } else { 0x00 }
            }
            _ => 0,
        }
    }

    fn set_frame_counter(&mut self, val: u8) {
        self.frame_period = 4 + (val >> 7);
        self.frame_irq = (val & 0x40) == 0;
    }
    fn set_control(&mut self, val: u8) {
        self.pulse0.set_enabled((val & 0x01) != 0);
        self.pulse1.set_enabled((val & 0x02) != 0);
        self.triangle.set_enabled((val & 0x04) != 0);
        self.noise.set_enabled((val & 0x08) != 0);
        self.dmc.set_enabled((val & 0x10) != 0);
    }
}
