use std::default::Default;

const LENGTH_TABLE: [u8; 32] = [
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
];

const TRIANGLE: [u8; 32] = [
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8 ,9, 10, 11, 12, 13, 14, 15,
];

#[derive(Clone, Debug, Default)]
pub struct Registers {
    control: u8,
    timer_lo: u8,
    timer_hi: u8,
}

#[derive(Clone, Debug, Default)]
pub struct ApuTriangle {
    output_volume: f32,

    enabled: bool,
    length_enabled: bool,
    length_value: u8,

    timer_period: u16,
    timer_value: u16,

    duty_mode: usize,
    duty_value: usize,

    counter_reload: bool,
    counter_period: u8,
    counter_value: u8,

    reg: Registers,
}


impl ApuTriangle {
    pub fn new(volume: f32) -> Self {
        ApuTriangle {
            output_volume: volume,
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
        self.length_enabled = (val & 0x80) == 0;
        self.counter_period = val & 0x7f;
    }
    pub fn set_timer_low(&mut self, val: u8) {
        self.reg.timer_lo = val;
        self.timer_period = (self.timer_period & 0xFF00) | (val as u16);
    }
    pub fn set_timer_high(&mut self, val: u8) {
        self.reg.timer_hi = val;
        self.length_value = LENGTH_TABLE[(val >> 3) as usize];
        self.timer_period = (self.timer_period & 0x00FF) |
                            ((val & 0x07) as u16) << 8;
        self.timer_value = self.timer_period;
        self.counter_reload = true;
    }

    pub fn output(&self) -> f32 {
        let val = self.internal_output();
        self.output_volume * (val as f32) / 16.0f32
    }

    fn internal_output(&self) -> u8 {
        if self.enabled == false ||
           self.length_value == 0 ||
           self.counter_value == 0 ||
           (self.timer_period == 0 && self.timer_value == 0) {
            0
        } else {
            TRIANGLE[self.duty_value]
        }
    }

    pub fn step_timer(&mut self) {
        if self.timer_value == 0 {
            self.timer_value = self.timer_period;
            if self.length_value > 0 && self.counter_value > 0 {
                self.duty_value = (self.duty_value + 1) % 32;
            }
        } else {
            self.timer_value -= 1;
        }
    }
    pub fn step_length(&mut self) {
        if self.length_enabled && self.length_value > 0 {
            self.length_value -= 1;
        }
    }
    pub fn step_counter(&mut self) {
        if self.counter_reload {
            self.counter_value = self.counter_period;
        } else if self.counter_value > 0 {
            self.counter_value -= 1;
        }
        if self.length_enabled {
            self.counter_reload = false;
        }
    }
}
