use anyhow::{anyhow, Result};
use pyo3::prelude::*;
use sdl2;
use std::cell::RefCell;
use std::path::{Path, PathBuf};
use structopt::StructOpt;
use thiserror::Error;

#[derive(Error, Debug)]
pub enum AppError {
    #[error("SDL Error")]
    SdlError(String),
}

#[derive(StructOpt, Debug)]
#[structopt(name = "protones")]
pub struct CommandlineArgs {
    #[structopt(short, long, default_value = "1280")]
    pub width: u32,
    #[structopt(short, long, default_value = "720")]
    pub height: u32,
    #[structopt(short, long, default_value = "0.5")]
    pub volume: f32,
    #[structopt(short, long, default_value = "4")]
    pub scale: f32,
    #[structopt(short, long, default_value = "1")]
    pub aspect: f32,
    #[structopt(long)]
    pub trace: bool,
    #[structopt(name = "FILE")]
    pub rom_file: Option<String>,
}

pub struct AppContext {
    pub args: CommandlineArgs,
    pub sdl: sdl2::Sdl,
    pub video: sdl2::VideoSubsystem,
    pub audio: sdl2::AudioSubsystem,
    pub window: sdl2::video::Window,
    pub gl: sdl2::video::GLContext,
    pub event_pump: RefCell<sdl2::EventPump>,
    pub gamecontroller: sdl2::GameControllerSubsystem,
    pub controller: Vec<sdl2::controller::GameController>,
    pub config_dir: PathBuf,
    pub data_dir: PathBuf,
    pub install_dir: PathBuf,
}

static mut APP_CONTEXT: Option<AppContext> = None;
//static mut APPLICATION: Option<Py<App>> = None;

impl AppContext {
    pub fn init(
        args: CommandlineArgs,
        name: &str,
        config_dir: &Path,
        data_dir: &Path,
    ) -> Result<()> {
        let sdl = sdl2::init().map_err(AppError::SdlError)?;
        let video = sdl.video().map_err(AppError::SdlError)?;
        {
            let gl_attr = video.gl_attr();
            gl_attr.set_context_profile(sdl2::video::GLProfile::Core);
            gl_attr.set_context_version(3, 0)
        }
        let window = video
            .window(name, args.width, args.height)
            .position_centered()
            .resizable()
            .opengl()
            .allow_highdpi()
            .build()?;

        let event_pump = sdl.event_pump().map_err(AppError::SdlError)?;
        let gl_context = window.gl_create_context().map_err(AppError::SdlError)?;
        gl::load_with(|s| video.gl_get_proc_address(s) as _);

        let audio = sdl.audio().map_err(AppError::SdlError)?;
        let gcss = sdl.game_controller().map_err(AppError::SdlError)?;
        let controllers = AppContext::open_controllers(&gcss)?;

        let config_dir = config_dir.to_path_buf();
        let install_dir = AppContext::installation_dir().expect("AppContext::init");

        unsafe {
            APP_CONTEXT = Some(AppContext {
                args: args,
                sdl: sdl,
                video: video,
                audio: audio,
                window: window,
                gl: gl_context,
                event_pump: RefCell::new(event_pump),
                gamecontroller: gcss,
                controller: controllers,
                config_dir: config_dir,
                data_dir: data_dir.to_path_buf(),
                install_dir: install_dir,
            });
        }
        Ok(())
    }

    fn open_controllers(
        gcss: &sdl2::GameControllerSubsystem,
    ) -> Result<Vec<sdl2::controller::GameController>> {
        let builtin_controller_db = include_bytes!("../../resources/gamecontrollerdb.txt");
        let mut reader = Vec::new();
        reader.extend_from_slice(builtin_controller_db);
        gcss.load_mappings_from_read(&mut &reader[..])?;

        let mut controllers = Vec::new();
        for i in 0..gcss.num_joysticks().map_err(AppError::SdlError)? {
            let name = gcss.name_for_index(i).unwrap_or("unknown".to_owned());
            if gcss.is_game_controller(i) {
                info!("Opening controller id {}: {}", i, name);
                controllers.push(gcss.open(i).unwrap());
            } else {
                info!("Skipping joystick id {}: {}", i, name);
            }
        }
        Ok(controllers)
    }

    pub fn installation_dir() -> Result<PathBuf> {
        let release = format!("target{}release", std::path::MAIN_SEPARATOR);
        let debug = format!("target{}debug", std::path::MAIN_SEPARATOR);
        let exe = std::env::current_exe()?;
        if let Some(parent) = exe.parent() {
            if parent.ends_with(release) || parent.ends_with(debug) {
                let mut dir = parent.to_path_buf();
                dir.pop();
                dir.pop();
                Ok(dir)
            } else {
                Ok(parent.to_path_buf())
            }
        } else {
            Err(anyhow!("No parent directory: {:?}", exe))
        }
    }

    pub fn setup_pythonsite(py: Python) -> Result<()> {
        let mut pydir = AppContext::installation_dir()?;
        pydir.push("python");
        info!("Configuring python sitedir: {:?}", pydir);
        let module = PyModule::import(py, "site")?;
        if let Some(dir) = pydir.to_str() {
            module.call("addsitedir", (dir,), None)?;
            Ok(())
        } else {
            Err(anyhow!("Could not convert directory name: {:?}", pydir))
        }
    }

    pub fn get() -> &'static AppContext {
        unsafe { APP_CONTEXT.as_ref().unwrap() }
    }

    //pub fn init_app(app: Py<App>) {
    //    unsafe {
    //        APPLICATION = Some(app);
    //    }
    //}

    //pub fn app() -> &'static Py<App> {
    //    unsafe { APPLICATION.as_ref().unwrap() }
    //}

    pub fn config_file<P: AsRef<Path>>(path: P) -> PathBuf {
        AppContext::get().config_dir.join(path)
    }

    pub fn data_file<P: AsRef<Path>>(path: P) -> PathBuf {
        AppContext::get().data_dir.join(path)
    }
}
