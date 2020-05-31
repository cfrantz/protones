use imgui;
use imgui::im_str;

use crate::gui::glhelper;
use crate::nes::nes::Nes;

#[derive(Clone, Debug)]
pub struct FooDebug {
    pub visible: bool,
}

impl FooDebug {
    pub fn new() -> Self {
        FooDebug {
            visible: false,
        }
    }

    pub fn draw(&mut self, nes: &Nes, ui: &imgui::Ui) {
        if !self.visible {
            return;
        }
        let mut visible =  self.visible;
        imgui::Window::new(im_str!("Foo"))
            .opened(&mut visible)
            .build(&ui, || {
            });
        self.visible = visible;
    }
}
