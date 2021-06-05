extern crate anyhow;
#[macro_use]
extern crate bitflags;
#[macro_use]
extern crate lazy_static;
#[macro_use]
extern crate log;
extern crate ron;
extern crate structopt;

extern crate directories;
extern crate env_logger;
extern crate gl;
extern crate imgui;
extern crate imgui_opengl_renderer;
extern crate imgui_sdl2;
extern crate memmap;
extern crate nfd;
extern crate pyo3;
extern crate sdl2;
extern crate typetag;
use directories::ProjectDirs;

pub mod gui;
pub mod nes;
pub mod util;

use anyhow::Result;
use gui::app::App;
use log::LevelFilter;
use pyo3::prelude::*;
use std::fs;
use std::io;
use structopt::StructOpt;
use util::app_context::{AppContext, CommandlineArgs};
use util::TerminalGuard;

fn run(py: Python) -> Result<()> {
    // Get our subdir added into python's import machinery.
    AppContext::setup_pythonsite(py)?;

    // Create the emulator application instance.
    let app = Py::new(py, App::new(py)?)?;

    // Add the application class & instance to the protones module.
    let module = PyModule::import(py, "protones")?;
    module.add_class::<App>()?;
    module.setattr("app", &app)?;

    // Set various preferences from command line args.
    let args = &AppContext::get().args;
    {
        let mut app = app.borrow_mut(py);
        app.trace = args.trace;
        app.preferences.volume = args.volume;
        app.preferences.scale = args.scale;
        app.preferences.aspect = args.aspect;
    }

    // Load the ROM specified on the command line.
    if let Some(rom) = &args.rom_file {
        app.borrow(py).load(py, rom)?;
    }

    // Run!
    App::run(&app, py);
    Ok(())
}

fn main() -> Result<()> {
    let args = CommandlineArgs::from_args();
    let mut builder = env_logger::Builder::from_default_env();
    builder.filter(None, LevelFilter::Info).init();

    let dirs = ProjectDirs::from("org", "CF207", "ProtoNES");
    if dirs.is_none() {
        return Err(
            io::Error::new(io::ErrorKind::NotFound, "Could not find home directory").into(),
        );
    }
    let dirs = dirs.unwrap();

    let config = dirs.config_dir();
    info!("Config dir: {}", config.display());
    fs::create_dir_all(config)?;

    let data = dirs.data_dir();
    info!("Data dir: {}", data.display());
    fs::create_dir_all(data)?;

    AppContext::init(args, "ProtoNES", config, data)?;
    let _guard = TerminalGuard::new();
    Python::with_gil(|py| run(py))
}
