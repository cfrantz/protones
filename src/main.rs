#[macro_use]
extern crate bitflags;
#[macro_use]
extern crate lazy_static;
#[macro_use]
extern crate log;
//#[macro_use]
extern crate structopt;


extern crate simplelog;
extern crate sdl2;
extern crate imgui;
extern crate imgui_sdl2;
extern crate gl;
extern crate imgui_opengl_renderer;
extern crate nfd;

pub mod gui;
pub mod nes;

use gui::app::App;
use structopt::StructOpt;
use simplelog::*;

#[derive(StructOpt, Debug)]
#[structopt(name = "protones")]
struct Opt {
    #[structopt(short, long, default_value="1280")]
    width: u32,

    #[structopt(short, long, default_value="720")]
    height: u32,

    #[structopt(long)]
    trace: bool,

    #[structopt(name="FILE")]
    rom_file: Option<String>,
}

fn main() {
    let opt = Opt::from_args();
    CombinedLogger::init(vec![
        TermLogger::new(LevelFilter::Info, Config::default(), TerminalMode::Mixed).unwrap(),
    ]).unwrap();

    let mut app = App::new("ProtoNES (rust)", opt.width, opt.height).unwrap();
    app.trace = opt.trace;
    if let Some(rom) = opt.rom_file {
        app.load(&rom).unwrap();
    }
    app.run();
}
