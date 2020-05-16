use crate::nes::nes::Nes;

pub trait Mapper {
    fn read(&mut self, address: u16) -> u8;
    fn write(&mut self, address: u16, value: u8);
    fn emulate(&mut self, nes: &mut Nes) {}
    fn expansion_audio(&mut self, nes: &mut Nes) -> f32 { 0.0f32 }
}
