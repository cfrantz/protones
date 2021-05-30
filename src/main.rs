#[macro_use]
extern crate bitflags;
#[macro_use]
extern crate lazy_static;
#[macro_use]
extern crate log;
extern crate ron;
//#[macro_use]
extern crate structopt;

extern crate directories;
extern crate gl;
extern crate imgui;
extern crate imgui_opengl_renderer;
extern crate imgui_sdl2;
extern crate memmap;
extern crate nfd;
extern crate sdl2;
extern crate env_logger;
extern crate typetag;
use directories::ProjectDirs;

pub mod gui;
pub mod nes;

use log::LevelFilter;
use gui::app::App;
use std::fs;
use std::io;
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

fn main() -> Result<(), io::Error> {
    let opt = Opt::from_args();
    let mut builder = env_logger::Builder::from_default_env();
    builder.filter(None, LevelFilter::Info).init();

    let dirs = ProjectDirs::from("org", "CF207", "ProtoNES");
    if dirs.is_none() {
        return Err(io::Error::new(
            io::ErrorKind::NotFound,
            "Could not find home directory",
        ));
    }
    let dirs = dirs.unwrap();

    let config = dirs.config_dir();
    info!("Config dir: {}", config.display());
    fs::create_dir_all(config)?;

    let data = dirs.data_dir();
    info!("Data dir: {}", data.display());
    fs::create_dir_all(data)?;

    let mut app = App::new("ProtoNES (rust)", opt.width, opt.height, config, data).unwrap();
    app.trace = opt.trace;
    app.preferences.volume = opt.volume;
    app.preferences.scale = opt.scale;
    app.preferences.aspect = opt.aspect;

    if let Some(rom) = opt.rom_file {
        app.load(&rom)?;
    }
    app.run();
    Ok(())
}
