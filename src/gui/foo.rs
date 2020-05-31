use imgui;
use imgui::im_str;

use crate::gui::glhelper;
use crate::nes::nes::Nes;

#[derive(Clone, Debug)]
pub struct APUDebug {
    pub visible: bool,
}

impl APUDebug {
    pub fn new() -> Self {
        APUDebug {
            visible: false,
        }
    }

    pub fn draw(&mut self, nes: &Nes, ui: &imgui::Ui) {
        if !self.visible {
            return;
        }
        let mut visible =  self.visible;
        imgui::Window::new(im_str!("Audio"))
            .opened(&mut visible)
            .build(&ui, || {
            });
        self.visible = visible;
    }
}
