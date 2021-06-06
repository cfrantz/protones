use anyhow::{anyhow, Result};
use imgui;
use imgui::im_str;
use imgui::MenuItem;
use imgui_opengl_renderer::Renderer;
use imgui_sdl2::ImguiSdl2;
use log::{error, info};
use nfd;
use pyo3::exceptions::PyException;
use pyo3::prelude::*;
use sdl2;
use sdl2::audio::{AudioCallback, AudioDevice, AudioSpecDesired};
use std::cell::{Cell, RefCell};
use std::fs;
use std::io;
use std::path::PathBuf;
use std::sync::mpsc;
use std::time::Instant;

use crate::gui::apu::ApuDebug;
use crate::gui::console::Console;
use crate::gui::controller::ControllerDebug;
use crate::gui::glhelper;
use crate::gui::hwpalette::hwpalette_editor;
use crate::gui::input::SdlInput;
use crate::gui::input::{Command, CommandKey, Keybinds};
use crate::gui::ppu::PpuDebug;
use crate::gui::preferences::Preferences;
use crate::nes::nes::Nes;
use crate::util::app_context::{AppContext, AppError};
use crate::util::pyexec::PythonExecutor;
use ron;

struct Playback {
    volume: f32,
    queue: mpsc::Receiver<Vec<f32>>,
}

impl AudioCallback for Playback {
    type Channel = f32;
    fn callback(&mut self, output: &mut [f32]) {
        match self.queue.try_recv() {
            Ok(values) => {
                for (sample, value) in output.iter_mut().zip(values) {
                    *sample = value * self.volume;
                }
            }
            Err(_) => {
                info!("audio underrun!");
                for sample in output.iter_mut() {
                    *sample = 0.0;
                }
            }
        }
    }
}

#[pyclass(unsendable)]
pub struct App {
    running: Cell<bool>,
    playback: AudioDevice<Playback>,
    queue: mpsc::SyncSender<Vec<f32>>,
    console: RefCell<Console>,
    executor: RefCell<PythonExecutor>,
    palette_editor: bool,

    nes: RefCell<Py<Nes>>,
    peer: RefCell<PyObject>,
    nes_image: imgui::TextureId,
    pub trace: bool,

    keybinds: Keybinds,
    pub preferences: Preferences,
    controller_debug: ControllerDebug,
    ppu_debug: PpuDebug,
    apu_debug: ApuDebug,

    nesfile: RefCell<Option<PathBuf>>,
    sramfile: RefCell<Option<PathBuf>>,

    state_slot: Cell<u32>,
}

impl App {
    pub fn new(py: Python) -> Result<Self> {
        let context = AppContext::get();
        let want_spec = AudioSpecDesired {
            freq: Some(48000),
            channels: Some(1),
            samples: Some(1024),
        };
        let (sender, receiver) = mpsc::sync_channel(2);
        let playback = context
            .audio
            .open_playback(None, &want_spec, |_spec| Playback {
                volume: 1.0,
                queue: receiver,
            })
            .map_err(AppError::SdlError)?;
        playback.resume();

        let module = PyModule::import(py, "protones")?;
        let peer = PyObject::from(module.call0("EmulatorPeer")?);

        Ok(App {
            running: Cell::new(false),
            playback: playback,
            queue: sender,
            console: RefCell::new(Console::new("Debug Console")),
            executor: RefCell::new(PythonExecutor::new(py)?),
            palette_editor: false,
            nes: RefCell::new(Py::new(py, Nes::blank())?),
            peer: RefCell::new(peer.clone_ref(py)),
            nes_image: glhelper::new_blank_image(256, 240),
            trace: false,
            keybinds: Keybinds::default(),
            preferences: Preferences::new(),
            controller_debug: ControllerDebug::new(),
            ppu_debug: PpuDebug::new(),
            apu_debug: ApuDebug::new(),
            nesfile: RefCell::default(),
            sramfile: RefCell::default(),
            state_slot: Cell::new(1),
        })
    }

    pub fn load(&self, py: Python, filename: &str) -> Result<()> {
        info!("Loading {}", filename);
        let path = PathBuf::from(filename);
        if path.is_file() {
            let sramname = path.with_extension("nes.sram");
            let basename = sramname.file_name().unwrap();
            let nes = Py::new(py, Nes::from_file(filename)?)?;

            let sramfile = AppContext::data_file(basename);
            match nes.borrow(py).mapper.borrow_mut().sram_load(&sramfile) {
                Ok(()) => {}
                Err(e) => info!("Could not load SRAM file {:?}: {:?}", sramfile, e),
            }

            nes.borrow(py).reset();
            nes.borrow(py).trace.set(self.trace);
            self.nes.replace(nes);
            self.nesfile.replace(Some(path));
            self.sramfile.replace(Some(sramfile));
            Ok(())
        } else {
            Err(io::Error::new(io::ErrorKind::NotFound, filename).into())
        }
    }

    fn loader(&self, py: Python) -> Result<()> {
        let result = nfd::open_file_dialog(None, None).unwrap();
        match result {
            nfd::Response::Okay(path) => self.load(py, &path),
            _ => Ok(()),
        }
    }

    fn sram_save(&self, py: Python) {
        if let Some(sramfile) = &*self.sramfile.borrow() {
            // Save the SRAM file every second.
            let refnes = self.nes.borrow();
            if refnes.borrow(py).frame.get() % 60 == 0 {
                match refnes.borrow(py).mapper.borrow().sram_save(sramfile) {
                    Ok(()) => {}
                    Err(e) => error!("Could not save SRAM file {:?}: {:?}", sramfile, e),
                }
            }
        }
    }

    fn draw_nes(&mut self, py: Python, ui: &imgui::Ui) {
        self.sram_save(py);
        let refnes = self.nes.borrow();

        {
            let refpeer = self.peer.borrow();
            match refpeer.call_method1(py, "emulate_frame", (refnes.clone_ref(py),)) {
                Ok(_) => {}
                Err(e) => {
                    error!("Error calling `emulate_frame` on peer object: {:?}", e);
                }
            }
        }
        let nes = refnes.borrow(py);
        let mut audio = nes.audio.borrow_mut();
        if audio.len() >= 1024 {
            let frag: Vec<_> = audio.drain(0..1024).collect();
            self.queue.send(frag).unwrap();
        }

        glhelper::update_image(self.nes_image, 0, 0, 256, 240, &nes.ppu.borrow().picture);
        let size = [
            256.0 * self.preferences.scale * self.preferences.aspect,
            240.0 * self.preferences.scale,
        ];
        let style = ui.push_style_vars(&[
            imgui::StyleVar::WindowPadding([0.0, 0.0]),
            imgui::StyleVar::WindowRounding(0.0),
            imgui::StyleVar::WindowBorderSize(0.0),
        ]);
        imgui::Window::new(im_str!("NES"))
            .position([0.0, 20.0], imgui::Condition::Always)
            .size(size, imgui::Condition::Always)
            .flags(
                imgui::WindowFlags::NO_TITLE_BAR
                 | imgui::WindowFlags::NO_MOVE
                 | imgui::WindowFlags::NO_SCROLLBAR
                 | imgui::WindowFlags::NO_BRING_TO_FRONT_ON_FOCUS
                 // Imgui-rs 0.4.0
                 //| imgui::WindowFlags::NO_SROLL_WITH_MOUSE
                 | imgui::WindowFlags::NO_RESIZE,
            )
            .build(ui, || {
                imgui::Image::new(self.nes_image, size).build(ui);
            });
        style.pop(&ui);
    }

    fn draw_console(&self, ui: &imgui::Ui) {
        let mut console = self.console.borrow_mut();
        let mut executor = self.executor.borrow_mut();
        console.draw(&mut *executor, ui);
    }

    fn draw(&mut self, py: Python, ui: &imgui::Ui) {
        ui.main_menu_bar(|| {
            ui.menu(im_str!("File"), true, || {
                if MenuItem::new(im_str!("Open")).build(ui) {
                    if let Err(e) = self.loader(py) {
                        error!("Could not load file: {:?}", e);
                    }
                }
                if MenuItem::new(im_str!("Quit")).build(ui) {
                    self.running.set(false);
                }
            });
            ui.menu(im_str!("Edit"), true, || {
                MenuItem::new(im_str!("Hardware Palette"))
                    .build_with_ref(ui, &mut self.palette_editor);
                MenuItem::new(im_str!("Preferences"))
                    .build_with_ref(ui, &mut self.preferences.visible);
            });
            ui.menu(im_str!("View"), true, || {
                MenuItem::new(im_str!("Debug Console"))
                    .build_with_ref(ui, &mut self.console.borrow_mut().visible);
                MenuItem::new(im_str!("Audio")).build_with_ref(ui, &mut self.apu_debug.visible);
                MenuItem::new(im_str!("Controllers"))
                    .build_with_ref(ui, &mut self.controller_debug.visible);
                MenuItem::new(im_str!("CHR Viewer"))
                    .build_with_ref(ui, &mut self.ppu_debug.chr_visible);
                MenuItem::new(im_str!("VRAM Viewer"))
                    .build_with_ref(ui, &mut self.ppu_debug.vram_visible);
            })
        });

        {
            let refnes = self.nes.borrow();
            let nes = refnes.borrow(py);
            self.preferences.draw(ui);
            self.apu_debug.draw(&nes, ui);
            self.controller_debug.draw(&nes, ui);
            self.ppu_debug.draw(&nes, ui);

            if self.palette_editor {
                let visible = &mut self.palette_editor;
                let pal = &mut nes.palette.borrow_mut();
                hwpalette_editor(pal, ui, visible);
            }
        }
        self.draw_nes(py, ui);
        {
            let mut playback = self.playback.lock();
            playback.volume = self.preferences.volume;
        }
    }

    fn save_state(&self, py: Python) {
        if let Some(sramfile) = &*self.sramfile.borrow() {
            let refnes = self.nes.borrow();
            let nes = refnes.borrow(py);
            let pretty = ron::ser::PrettyConfig::new();
            let s = ron::ser::to_string_pretty(&*nes, pretty).unwrap();
            let statefile = sramfile.with_extension(format!("state.{}", self.state_slot.get()));
            info!("Saving state to {:?}", statefile);
            match fs::write(statefile, &s) {
                Ok(()) => {}
                Err(e) => error!("Could not save state: {:?}", e),
            }
        }
    }

    fn load_state(&self, py: Python) -> Result<Py<Nes>> {
        if let Some(sramfile) = &*self.sramfile.borrow() {
            let statefile = sramfile.with_extension(format!("state.{}", self.state_slot.get()));
            info!("Loading state from {:?}", statefile);
            let file = fs::File::open(statefile)?;
            let nes: Nes = ron::de::from_reader(&file)?;
            Ok(Py::new(py, nes)?)
        } else {
            Err(anyhow!("No game loaded."))
        }
    }

    pub fn run(slf: &Py<Self>, py: Python) {
        let context = AppContext::get();
        let mut last_frame = Instant::now();
        let mut imgui = imgui::Context::create();
        let mut imgui_sdl2 = ImguiSdl2::new(&mut imgui, &context.window);
        let renderer = Renderer::new(&mut imgui, |s| context.video.gl_get_proc_address(s) as _);

        slf.borrow(py).running.set(true);
        'running: loop {
            if !slf.borrow(py).running.get() {
                break 'running;
            }
            let frame_start = Instant::now();
            let mut want_load = false;
            let mut want_save = false;

            let mut event_pump = context.event_pump.borrow_mut();
            for event in event_pump.poll_iter() {
                imgui_sdl2.handle_event(&mut imgui, &event);
                if imgui_sdl2.ignore_event(&event) {
                    continue;
                }
                let command = slf.borrow(py).keybinds.translate(&event);
                {
                    let slf = slf.borrow(py);
                    let refnes = slf.nes.borrow();
                    let nes = refnes.borrow(py);
                    nes.controller[0].borrow_mut().process_event(&event);
                    nes.controller[0].borrow_mut().process_command(&command);
                }
                match command {
                    Command::Up(k) => match k {
                        CommandKey::SystemQuit => {
                            break 'running;
                        }
                        CommandKey::SystemSaveState => {
                            want_save = true;
                        }
                        CommandKey::SystemRestoreState => {
                            want_load = true;
                        }
                        CommandKey::SystemReset => {
                            let slf = slf.borrow(py);
                            let refnes = slf.nes.borrow();
                            let nes = refnes.borrow(py);
                            nes.reset();
                        }
                        CommandKey::SystemPause => {
                            let slf = slf.borrow(py);
                            let refnes = slf.nes.borrow();
                            let nes = refnes.borrow(py);
                            nes.pause.set(!nes.pause.get());
                        }
                        CommandKey::SystemFrameStep => {
                            let slf = slf.borrow(py);
                            let refnes = slf.nes.borrow();
                            let nes = refnes.borrow(py);
                            nes.pause.set(true);
                            nes.framestep.set(true);
                        }
                        CommandKey::SelectState0 => {
                            slf.borrow(py).state_slot.set(0);
                        }
                        CommandKey::SelectState1 => {
                            slf.borrow(py).state_slot.set(1);
                        }
                        CommandKey::SelectState2 => {
                            slf.borrow(py).state_slot.set(2);
                        }
                        CommandKey::SelectState3 => {
                            slf.borrow(py).state_slot.set(3);
                        }
                        CommandKey::SelectState4 => {
                            slf.borrow(py).state_slot.set(4);
                        }
                        CommandKey::SelectState5 => {
                            slf.borrow(py).state_slot.set(5);
                        }
                        CommandKey::SelectState6 => {
                            slf.borrow(py).state_slot.set(6);
                        }
                        CommandKey::SelectState7 => {
                            slf.borrow(py).state_slot.set(7);
                        }
                        CommandKey::SelectState8 => {
                            slf.borrow(py).state_slot.set(8);
                        }
                        CommandKey::SelectState9 => {
                            slf.borrow(py).state_slot.set(9);
                        }
                        _ => {}
                    },
                    _ => {}
                }
            }

            if want_save {
                slf.borrow(py).save_state(py);
            }
            if want_load {
                let slf = slf.borrow(py);
                match slf.load_state(py) {
                    Ok(nes) => {
                        {
                            let refnes = slf.nes.borrow();
                            let old = refnes.borrow(py);
                            nes.borrow(py)
                                .mapper
                                .borrow_mut()
                                .set_cartridge(old.mapper.borrow().borrow_cart().clone());
                        }
                        slf.nes.replace(nes);
                    }
                    Err(e) => error!("Could not load state {}: {:?}", slf.state_slot.get(), e),
                }
            }

            imgui_sdl2.prepare_frame(imgui.io_mut(), &context.window, &event_pump.mouse_state());

            let now = Instant::now();
            let delta = now - last_frame;
            imgui.io_mut().delta_time = delta.as_secs_f32();
            last_frame = now;

            let ui = imgui.frame();
            slf.borrow_mut(py).draw(py, &ui);
            slf.borrow(py).draw_console(&ui);

            glhelper::clear_screen(&slf.borrow(py).preferences.background);
            imgui_sdl2.prepare_render(&ui, &context.window);
            renderer.render(ui);
            context.window.gl_swap_window();
            let frame_end = Instant::now();
            let delta = (frame_end - frame_start).as_secs_f64();
            slf.borrow_mut(py).preferences.set_frame_time(delta as f32);

            let leftover = (1.0 / Nes::FPS) - delta;
            if leftover > 0.0 {
                //thread::sleep(Duration::from_secs_f64(leftover));
            } else {
                //info!("Timing underrun: {:.03}us", leftover * 1e6);
            }
        }
    }
}

#[pymethods]
impl App {
    #[getter]
    fn get_running(&self) -> bool {
        self.running.get()
    }

    #[setter]
    fn set_running(&self, value: bool) {
        self.running.set(value)
    }

    #[name = "load"]
    fn py_load(&self, py: Python, filename: &str) -> PyResult<()> {
        self.load(py, filename)
            .map_err(|e| PyException::new_err(e.to_string()))
    }

    #[getter]
    fn get_nes(&self, py: Python) -> Py<Nes> {
        self.nes.borrow().clone_ref(py)
    }

    #[getter]
    fn get_peer(&self, py: Python) -> PyObject {
        self.peer.borrow().clone_ref(py)
    }

    #[setter]
    fn set_peer(&self, peer: PyObject) {
        self.peer.replace(peer);
    }
}
