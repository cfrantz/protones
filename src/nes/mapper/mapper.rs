use crate::nes::nes::Nes;

pub fn simple_mirror_address(mode: u8, address: u16) -> u16 {
    let address = address & 0xFFF;
    match mode {
        0 => {
            let a11 = address & 0x800;
            (address & !0xC00) | (a11 >> 1)
        }
        1 => address & !0x800,
        2 => address & !0xC00,
        3 => (address & !0xC00) | 0x400,
        4 => address,
        _ => {
            panic!("Unknown mirror mode {}", mode);
        }
    }
}

pub trait Mapper {
    fn read(&mut self, address: u16) -> u8;
    fn write(&mut self, address: u16, value: u8);
    fn emulate(&mut self, _nes: &Nes) {}
    fn expansion_audio(&mut self, _nes: &Nes) -> f32 {
        0.0f32
    }
    fn mirror_address(&self, address: u16) -> u16;
}
