use imgui;
use imgui::im_str;

use crate::nes::controller::Controller;
use crate::nes::nes::Nes;

#[derive(Clone, Debug)]
pub struct ControllerDebug {
    pub visible: bool,
}

const ON: [f32; 4] = [1.0, 1.0, 1.0, 1.0];
const OFF: [f32; 4] = [0.5, 0.5, 0.5, 1.0];

fn color(active: u8) -> [f32; 4] {
    if active == 0 {
        OFF
    } else {
        ON
    }
}

impl ControllerDebug {
    pub fn new() -> Self {
        ControllerDebug { visible: false }
    }

    fn draw_controller(&self, num: usize, c: &Controller, ui: &imgui::Ui) {
        ui.group(|| {
            ui.text(format!("Controller {}", num));
            ui.text_colored(color(c.buttons & Controller::BUTTON_UP), " U");
            ui.text_colored(color(c.buttons & Controller::BUTTON_LEFT), "L");
            ui.same_line(0.0);
            ui.text_colored(color(c.buttons & Controller::BUTTON_RIGHT), "R");
            ui.same_line(0.0);
            ui.text_colored(color(c.buttons & Controller::BUTTON_SELECT), "Sel");
            ui.same_line(0.0);
            ui.text_colored(color(c.buttons & Controller::BUTTON_START), "Sta");
            ui.same_line(0.0);
            ui.text_colored(color(c.buttons & Controller::BUTTON_B), " B");
            ui.same_line(0.0);
            ui.text_colored(color(c.buttons & Controller::BUTTON_A), "A");
            ui.text_colored(color(c.buttons & Controller::BUTTON_DOWN), " D");
        });
    }

    pub fn draw(&mut self, nes: &Nes, ui: &imgui::Ui) {
        if !self.visible {
            return;
        }
        let mut visible = self.visible;
        imgui::Window::new(im_str!("Controller"))
            .opened(&mut visible)
            .build(&ui, || {
                for i in 0..2 {
                    if i > 0 {
                        ui.same_line(0.0);
                        ui.text("    ");
                        ui.same_line(0.0);
                    }
                    let c = nes.controller[i].borrow();
                    self.draw_controller(i, &c, ui);
                }
            });
        self.visible = visible;
    }
}
