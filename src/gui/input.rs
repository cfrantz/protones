use sdl2::event::Event;
use sdl2::controller::Button;
use sdl2::controller::Axis;
use crate::nes::controller::Controller;

pub trait SdlInput {
    fn process_event(&mut self, event: &Event);
}

fn xbox_to_nes(xbox: &Button) -> u8 {
    match xbox {
        Button::A => Controller::BUTTON_B,
        Button::B => Controller::BUTTON_A,
        Button::Back => Controller::BUTTON_SELECT,
        Button::Start => Controller::BUTTON_START,
        Button::DPadLeft => Controller::BUTTON_LEFT,
        Button::DPadRight => Controller::BUTTON_RIGHT,
        Button::DPadUp => Controller::BUTTON_UP,
        Button::DPadDown => Controller::BUTTON_DOWN,
        _ => 0,
    }
}

impl SdlInput for Controller {
    fn process_event(&mut self, event: &Event) {
        match event {
            Event::ControllerButtonDown{ button: b, .. } => {
                self.buttons |= xbox_to_nes(b);
            },
            Event::ControllerButtonUp{ button: b, .. } => {
                self.buttons &= !xbox_to_nes(b);
            },
            Event::ControllerAxisMotion{ axis: Axis::LeftX, value: v, .. } => {
                if *v < -3000 {
                    self.buttons |= Controller::BUTTON_LEFT;
                    self.buttons &= !Controller::BUTTON_RIGHT;
                } else if *v > 3000 {
                    self.buttons &= !Controller::BUTTON_LEFT;
                    self.buttons |= Controller::BUTTON_RIGHT;
                } else {
                    self.buttons &= !Controller::BUTTON_LEFT;
                    self.buttons &= !Controller::BUTTON_RIGHT;
                }
            },
            Event::ControllerAxisMotion{ axis: Axis::LeftY, value: v, .. } => {
                if *v < -3000 {
                    self.buttons |= Controller::BUTTON_UP;
                    self.buttons &= !Controller::BUTTON_DOWN;
                } else if *v > 3000 {
                    self.buttons &= !Controller::BUTTON_UP;
                    self.buttons |= Controller::BUTTON_DOWN;
                } else {
                    self.buttons &= !Controller::BUTTON_UP;
                    self.buttons &= !Controller::BUTTON_DOWN;
                }
            },
            _ => {},
        }
    }
}
