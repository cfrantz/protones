use std::default::Default;

#[derive(Clone, Debug, Default)]
pub struct Controller {
    buttons: u8,
    index: u8,
    strobe: u8,
}

impl Controller {
    pub const BUTTON_A: u8 = 0x01;
    pub const BUTTON_B: u8 = 0x02;
    pub const BUTTON_SELECT: u8 = 0x04;
    pub const BUTTON_START: u8 = 0x08;
    pub const BUTTON_UP: u8 = 0x10;
    pub const BUTTON_DOWN: u8 = 0x20;
    pub const BUTTON_LEFT: u8 = 0x40;
    pub const BUTTON_RIGHT: u8 = 0x80;

    pub fn read(&mut self) -> u8 {
        let ret = if self.index < 8 {
            (self.buttons >> self.index) & 1
        } else {
            0
        };
        self.index += 1;
        if (self.strobe & 1) != 0{
            self.index = 0;
        }
        ret
    }

    pub fn write(&mut self, val: u8) {
        self.strobe = val;
        if (self.strobe & 1) != 0{
            self.index = 0;
        }
    }
}
