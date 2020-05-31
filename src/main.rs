#[macro_use]
extern crate bitflags;
#[macro_use]
extern crate lazy_static;
#[macro_use]
extern crate log;
//#[macro_use]
extern crate structopt;

extern crate gl;
extern crate imgui;
extern crate imgui_opengl_renderer;
extern crate imgui_sdl2;
extern crate nfd;
extern crate sdl2;
extern crate simplelog;

pub mod gui;
pub mod nes;

use gui::app::App;
use simplelog::*;
use structopt::StructOpt;

#[derive(StructOpt, Debug)]
#[structopt(name = "protones")]
struct Opt {
    #[structopt(short, long, default_value = "1280")]
    width: u32,

    #[structopt(short, long, default_value = "720")]
    height: u32,

    #[structopt(short, long, default_value = "0.5")]
    volume: f32,

    #[structopt(short, long, default_value = "4")]
    scale: f32,

    #[structopt(short, long, default_value = "1")]
    aspect: f32,

    #[structopt(long)]
    trace: bool,

    #[structopt(name = "FILE")]
    rom_file: Option<String>,
}

fn main() {
    let opt = Opt::from_args();
    CombinedLogger::init(vec![TermLogger::new(
        LevelFilter::Info,
        Config::default(),
        TerminalMode::Mixed,
    )
    .unwrap()])
    .unwrap();

    let mut app = App::new("ProtoNES (rust)", opt.width, opt.height).unwrap();
    app.trace = opt.trace;
    app.preferences.volume = opt.volume;
    app.preferences.scale = opt.scale;
    app.preferences.aspect = opt.aspect;

    if let Some(rom) = opt.rom_file {
        app.load(&rom).unwrap();
    }
    app.run();
}
