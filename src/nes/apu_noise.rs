use std::default::Default;

const NOISE_TABLE: [u16; 16] = [
    4, 8, 16, 32, 64, 96, 128, 160,
    202, 254, 380, 508, 762, 1016, 2034, 4068,
];

const LENGTH_TABLE: [u8; 32] = [
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
];

#[derive(Clone, Debug, Default)]
pub struct Registers {
    pub control: u8,
    pub period: u8,
    pub length: u8,
}

#[derive(Clone, Debug, Default)]
pub struct ApuNoise {
    pub output_volume: f32,

    enabled: bool,
    mode: bool,
    shift_register: u16,

    length_enabled: bool,
    length_value: u8,

    timer_period: u16,
    timer_value: u16,

    envelope_enable: bool,
    envelope_start: bool,
    envelope_loop: bool,
    envelope_period: u8,
    envelope_value: u8,
    envelope_volume: u8,

    constant_volume: u8,
    pub reg: Registers,
    pub dbg_offset: usize,
    pub dbg_buf: Vec<f32>,
}


impl ApuNoise {
    pub fn new(volume: f32) -> Self {
        ApuNoise {
            output_volume: volume,
            shift_register: 1,
            dbg_buf: vec![0f32; 1024],
            ..Default::default()
        }
    }
    pub fn active(&self) -> bool {
        self.length_value != 0
    }
    pub fn set_enabled(&mut self, val: bool) {
        self.enabled = val;
        if !self.enabled {
            self.length_value = 0;
        }
    }
    pub fn set_control(&mut self, val: u8) {
        self.reg.control = val;
        self.length_enabled = (val & 0x20) == 0;
        self.envelope_loop = (val & 0x20) != 0;
        self.envelope_enable = (val & 0x10) == 0;
        self.envelope_period = val & 0x0f;
        self.constant_volume = val & 0x0f;
        self.envelope_start = true;
    }
    pub fn set_period(&mut self, val: u8) {
        self.reg.period = val;
        self.mode = (val & 0x80) != 0;
        self.timer_period = NOISE_TABLE[(val & 0x0f) as usize];
    }
    pub fn set_length(&mut self, val: u8) {
        self.reg.length = val;
        self.length_value = LENGTH_TABLE[(val >> 3) as usize];
        self.envelope_start = true;
    }

    pub fn output(&mut self) -> f32 {
        let val = (self.internal_output() as f32) / 15.0;
        self.dbg_buf[self.dbg_offset] = val;
        self.dbg_offset = (self.dbg_offset + 1) % self.dbg_buf.len();
        self.output_volume * val
    }

    fn internal_output(&self) -> u8 {
        if self.enabled == false ||
           self.length_value == 0 ||
           (self.shift_register & 1) == 1 {
            0
        } else if self.envelope_enable {
            self.envelope_volume
        } else {
            self.constant_volume
        }
    }

    pub fn step_timer(&mut self) {
        if self.timer_value == 0 {
            self.timer_value = self.timer_period;
            let shift = if self.mode { 6 } else { 1 };
            let b1 = self.shift_register & 1;
            let b2 = (self.shift_register >> shift) & 1;
            self.shift_register = (self.shift_register >> 1) |
                                  ((b1 ^ b2) << 14);
        } else {
            self.timer_value -= 1;
        }
    }
    pub fn step_envelope(&mut self) {
        if self.envelope_start {
            self.envelope_volume = 15;
            self.envelope_value = self.envelope_period;
            self.envelope_start = false;
        } else if self.envelope_value > 0 {
            self.envelope_value -= 1;
        } else {
            if self.envelope_volume > 0 {
                self.envelope_volume -= 1;
            } else if self.envelope_loop {
                self.envelope_volume = 15;
            }
            self.envelope_value = self.envelope_period;
        }
    }
    pub fn step_length(&mut self) {
        if self.length_enabled && self.length_value > 0 {
            self.length_value -= 1;
        }
    }
}
