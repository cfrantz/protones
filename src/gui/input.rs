use crate::nes::controller::Controller;
use super::scancode::ScancodeRef;
use sdl2::controller::Axis;
use sdl2::controller::Button;
use sdl2::keyboard::Scancode;
use sdl2::event::Event;
use std::collections::HashMap;
use serde::{Serialize, Deserialize};
use std::default::Default;
use ron;

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum CommandKey {
    ControllerUp,
    ControllerDown,
    ControllerLeft,
    ControllerRight,
    ControllerSelect,
    ControllerStart,
    ControllerB,
    ControllerA,

    Controller2UpAndA,

    SystemQuit,
    SystemReset,
    SystemPause,
    SystemSaveState,
    SystemRestoreState,

    SelectState0,
    SelectState1,
    SelectState2,
    SelectState3,
    SelectState4,
    SelectState5,
    SelectState6,
    SelectState7,
    SelectState8,
    SelectState9,
}

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum Command {
    None,
    Down(CommandKey),
    Up(CommandKey),
}

// I couldn't figure out if/how to use Scancode directly in the
// HashMap as it's a foreign enum for which I had to derive
// serialization.  Wrapping it in the 'Key' enum like this at least
// makes it look nice when serialized to text with 'ron'.
#[derive(Hash, Eq, PartialEq, Debug, Clone, Serialize, Deserialize)]
pub enum Key {
    #[serde(with = "ScancodeRef")]
    Scancode(Scancode)
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Keybinds {
    pub keybind: HashMap<Key, CommandKey>,
}

impl Keybinds {
    pub fn translate(&self, event: &Event) -> Command {
        match event {
            Event::Quit {..} => {
                Command::Up(CommandKey::SystemQuit)
            }
            Event::KeyUp {scancode: Some(sc), ..} => {
                self.keybind.get(&Key::Scancode(*sc)).map_or_else(
                    || Command::None, |v| Command::Up(*v))
            }
            Event::KeyDown {scancode: Some(sc), ..} => {
                self.keybind.get(&Key::Scancode(*sc)).map_or_else(
                    || Command::None, |v| Command::Down(*v))
            }
            _ => Command::None
        }
    }
}

impl Default for Keybinds {
    fn default() -> Self {
        let builtin = include_bytes!("../../resources/default_keybinds.ron");
        ron::de::from_bytes(builtin).unwrap()
    }
}

pub trait SdlInput {
    fn process_event(&mut self, event: &Event);
    fn process_command(&mut self, command: &Command);
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

fn command_key_to_nes(ck: &CommandKey) -> u8 {
    match ck {
        CommandKey::ControllerB => Controller::BUTTON_B,
        CommandKey::ControllerA => Controller::BUTTON_A,
        CommandKey::ControllerSelect => Controller::BUTTON_SELECT,
        CommandKey::ControllerStart => Controller::BUTTON_START,
        CommandKey::ControllerLeft => Controller::BUTTON_LEFT,
        CommandKey::ControllerRight => Controller::BUTTON_RIGHT,
        CommandKey::ControllerUp => Controller::BUTTON_UP,
        CommandKey::ControllerDown => Controller::BUTTON_DOWN,
        _ => 0,
    }
}

impl SdlInput for Controller {
    fn process_event(&mut self, event: &Event) {
        match event {
            Event::ControllerButtonDown { button: b, .. } => {
                self.buttons |= xbox_to_nes(b);
            }
            Event::ControllerButtonUp { button: b, .. } => {
                self.buttons &= !xbox_to_nes(b);
            }
            Event::ControllerAxisMotion {
                axis: Axis::LeftX,
                value: v,
                ..
            } => {
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
            }
            Event::ControllerAxisMotion {
                axis: Axis::LeftY,
                value: v,
                ..
            } => {
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
            }
            _ => {}
        }
    }

    fn process_command(&mut self, command: &Command) {
        match command {
            Command::Down(key) => {
                self.buttons |= command_key_to_nes(key);
            }
            Command::Up(key) => {
                self.buttons &= !command_key_to_nes(key);
            }
            _ => {}
        }
    }
}
