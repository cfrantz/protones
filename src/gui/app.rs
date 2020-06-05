use imgui;
use imgui::im_str;
use imgui::MenuItem;
use imgui_opengl_renderer::Renderer;
use imgui_sdl2::ImguiSdl2;
use log::{error, info};
use nfd;
use sdl2;
use sdl2::audio::{AudioCallback, AudioDevice, AudioSpecDesired};
use sdl2::controller::GameController;
use std::fs;
use std::io;
use std::path::{Path, PathBuf};
use std::sync::mpsc;
use std::time::Instant;

use crate::gui::apu::ApuDebug;
use crate::gui::controller::ControllerDebug;
use crate::gui::glhelper;
use crate::gui::hwpalette::hwpalette_editor;
use crate::gui::input::SdlInput;
use crate::gui::input::{Command, CommandKey, Keybinds};
use crate::gui::ppu::PpuDebug;
use crate::gui::preferences::Preferences;
use crate::nes::nes::Nes;
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

pub struct App {
    #[allow(dead_code)] // keep SDL context.
    sdl_context: sdl2::Sdl,
    video: sdl2::VideoSubsystem,
    #[allow(dead_code)] // keep audio context.
    audio: sdl2::AudioSubsystem,
    #[allow(dead_code)] // keep audio playback device.
    playback: AudioDevice<Playback>,
    #[allow(dead_code)] // keep gamecontroller context.
    gamecontoller: sdl2::GameControllerSubsystem,
    window: sdl2::video::Window,
    #[allow(dead_code)] // keep GL context.
    gl_context: sdl2::video::GLContext,
    event_pump: sdl2::EventPump,
    #[allow(dead_code)] // keep list of open controllers.
    controllers: Vec<GameController>,
    queue: mpsc::SyncSender<Vec<f32>>,
    running: bool,
    palette_editor: bool,

    nes: Option<Box<Nes>>,
    nes_image: imgui::TextureId,
    pub trace: bool,

    keybinds: Keybinds,
    pub preferences: Preferences,
    controller_debug: ControllerDebug,
    ppu_debug: PpuDebug,
    apu_debug: ApuDebug,

    #[allow(dead_code)]
    config_dir: PathBuf,
    data_dir: PathBuf,

    nesfile: Option<PathBuf>,
    sramfile: Option<PathBuf>,

    state_slot: u32,
}

impl App {
    pub fn new(
        name: &str,
        width: u32,
        height: u32,
        config_dir: &Path,
        data_dir: &Path,
    ) -> Result<Self, String> {
        let sdl_context = sdl2::init()?;
        let video = sdl_context.video()?;
        {
            let gl_attr = video.gl_attr();
            gl_attr.set_context_profile(sdl2::video::GLProfile::Core);
            gl_attr.set_context_version(3, 0);
        }
        let window = video
            .window(name, width, height)
            .position_centered()
            .resizable()
            .opengl()
            .allow_highdpi()
            .build()
            .unwrap();

        let gcss = sdl_context.game_controller()?;

        let event_pump = sdl_context.event_pump()?;

        let gl_context = window.gl_create_context()?;
        gl::load_with(|s| video.gl_get_proc_address(s) as _);

        let audio = sdl_context.audio()?;
        let want_spec = AudioSpecDesired {
            freq: Some(48000),
            channels: Some(1),
            samples: Some(1024),
        };
        let (sender, receiver) = mpsc::sync_channel(2);
        let playback = audio.open_playback(None, &want_spec, |_spec| Playback {
            volume: 1.0,
            queue: receiver,
        })?;
        playback.resume();
        let controllers = App::open_controllers(&gcss)?;

        // FIXME(cfrantz): Does this do anything?
        //video.gl_set_swap_interval(1)?;

        Ok(App {
            sdl_context: sdl_context,
            video: video,
            audio: audio,
            playback: playback,
            gamecontoller: gcss,
            window: window,
            gl_context: gl_context,
            event_pump: event_pump,
            controllers: controllers,
            queue: sender,
            running: true,
            palette_editor: false,
            nes: None,
            nes_image: glhelper::new_blank_image(256, 240),
            trace: false,
            keybinds: Keybinds::default(),
            preferences: Preferences::new(),
            controller_debug: ControllerDebug::new(),
            ppu_debug: PpuDebug::new(),
            apu_debug: ApuDebug::new(),
            config_dir: config_dir.to_path_buf(),
            data_dir: data_dir.to_path_buf(),
            nesfile: None,
            sramfile: None,
            state_slot: 1,
        })
    }

    fn open_controllers(
        gcss: &sdl2::GameControllerSubsystem,
    ) -> Result<Vec<GameController>, String> {
        let builtin_controller_db = include_bytes!("../../resources/gamecontrollerdb.txt");
        let mut reader = Vec::new();
        reader.extend_from_slice(builtin_controller_db);
        gcss.load_mappings_from_read(&mut &reader[..]).unwrap();

        let mut controllers = Vec::<GameController>::new();
        for i in 0..gcss.num_joysticks()? {
            let name = gcss.name_for_index(i).unwrap_or(String::from("unknown"));
            if gcss.is_game_controller(i) {
                print!("Opening controller id {}: {}", i, name);
                controllers.push(gcss.open(i).unwrap());
            } else {
                print!("Skipping joystick id {}: {}", i, name);
            }
        }
        Ok(controllers)
    }

    pub fn load(&mut self, filename: &str) -> io::Result<()> {
        info!("Loading {}", filename);
        let path = PathBuf::from(filename);
        if path.is_file() {
            let sramname = path.with_extension("nes.sram");
            let basename = sramname.file_name().unwrap();
            let mut nes = Box::new(Nes::from_file(filename)?);

            let sramfile = self.data_dir.join(basename);
            match nes.mapper.borrow_mut().sram_load(&sramfile) {
                Ok(()) => {}
                Err(e) => info!("Could not load SRAM file {:?}: {:?}", sramfile, e),
            }

            nes.reset();
            nes.trace = self.trace;
            self.nes = Some(nes);
            self.nesfile = Some(path);
            self.sramfile = Some(sramfile);
            Ok(())
        } else {
            Err(io::Error::new(io::ErrorKind::NotFound, filename))
        }
    }

    fn loader(&mut self) -> io::Result<()> {
        let result = nfd::open_file_dialog(None, None).unwrap();
        match result {
            nfd::Response::Okay(path) => self.load(&path),
            _ => Ok(()),
        }
    }

    fn sram_save(&self) {
        if let (Some(nes), Some(sramfile)) = (&self.nes, &self.sramfile) {
            // Save the SRAM file every second.
            if nes.frame % 60 == 0 {
                match nes.mapper.borrow().sram_save(sramfile) {
                    Ok(()) => {}
                    Err(e) => error!("Could not save SRAM file {:?}: {:?}", sramfile, e),
                }
            }
        }
    }

    fn draw_nes(&mut self, ui: &imgui::Ui) {
        self.sram_save();
        if let Some(nes) = &mut self.nes {
            nes.emulate_frame();
            let mut audio = nes.audio.borrow_mut();
            if audio.len() >= 1024 {
                let frag: Vec<_> = audio.drain(0..1024).collect();
                self.queue.send(frag).unwrap();
            }
        }
        if let Some(nes) = &self.nes {
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
    }

    fn draw(&mut self, ui: &imgui::Ui) {
        ui.main_menu_bar(|| {
            ui.menu(im_str!("File"), true, || {
                if MenuItem::new(im_str!("Open")).build(ui) {
                    if let Err(e) = self.loader() {
                        error!("Could not load file: {:?}", e);
                    }
                }
                if MenuItem::new(im_str!("Quit")).build(ui) {
                    self.running = false;
                }
            });
            ui.menu(im_str!("Edit"), true, || {
                MenuItem::new(im_str!("Hardware Palette"))
                    .build_with_ref(ui, &mut self.palette_editor);
                MenuItem::new(im_str!("Preferences"))
                    .build_with_ref(ui, &mut self.preferences.visible);
            });
            ui.menu(im_str!("View"), true, || {
                MenuItem::new(im_str!("Audio")).build_with_ref(ui, &mut self.apu_debug.visible);
                MenuItem::new(im_str!("Controllers"))
                    .build_with_ref(ui, &mut self.controller_debug.visible);
                MenuItem::new(im_str!("CHR Viewer"))
                    .build_with_ref(ui, &mut self.ppu_debug.chr_visible);
                MenuItem::new(im_str!("VRAM Viewer"))
                    .build_with_ref(ui, &mut self.ppu_debug.vram_visible);
            })
        });

        self.preferences.draw(ui);
        if let Some(nes) = &self.nes {
            self.apu_debug.draw(nes, ui);
            self.controller_debug.draw(nes, ui);
            self.ppu_debug.draw(nes, ui);
        }
        if self.palette_editor {
            let visible = &mut self.palette_editor;
            if let Some(nes) = &self.nes {
                let pal = &mut nes.palette.borrow_mut();
                hwpalette_editor(pal, ui, visible);
            }
        }
        self.draw_nes(ui);
        {
            let mut playback = self.playback.lock();
            playback.volume = self.preferences.volume;
        }
    }

    fn save_state(&self) {
        if let (Some(nes), Some(sramfile)) = (&self.nes, &self.sramfile) {
            let pretty = ron::ser::PrettyConfig::new();
            let s = ron::ser::to_string_pretty(nes, pretty).unwrap();
            let statefile = sramfile.with_extension(format!("state.{}", self.state_slot));
            info!("Saving state to {:?}", statefile);
            match fs::write(statefile, &s) {
                Ok(()) => {}
                Err(e) => error!("Could not save state: {:?}", e),
            }
        }
    }

    fn load_state(&self) -> io::Result<Box<Nes>> {
        if let Some(sramfile) = &self.sramfile {
            let statefile = sramfile.with_extension(format!("state.{}", self.state_slot));
            info!("Loading state from {:?}", statefile);
            let file = fs::File::open(statefile)?;
            match ron::de::from_reader(&file) {
                Ok(nes) => Ok(Box::new(nes)),
                Err(v) => Err(io::Error::new(
                    io::ErrorKind::Other,
                    format!("Deserialization failure: {:?})", v),
                )),
            }
        } else {
            Err(io::Error::new(io::ErrorKind::Other, "No game loaded."))
        }
    }

    pub fn run(&mut self) {
        let mut last_frame = Instant::now();
        let mut imgui = imgui::Context::create();
        let mut imgui_sdl2 = ImguiSdl2::new(&mut imgui, &self.window);
        let renderer = Renderer::new(&mut imgui, |s| self.video.gl_get_proc_address(s) as _);

        'running: while self.running {
            let frame_start = Instant::now();
            let mut want_load = false;
            let mut want_save = false;

            for event in self.event_pump.poll_iter() {
                imgui_sdl2.handle_event(&mut imgui, &event);
                if imgui_sdl2.ignore_event(&event) {
                    continue;
                }
                let command = self.keybinds.translate(&event);
                if let Some(nes) = &self.nes {
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
                            if let Some(nes) = &self.nes {
                                nes.reset();
                            }
                        }
                        CommandKey::SystemPause => {
                            if let Some(nes) = &mut self.nes {
                                nes.pause = !nes.pause;
                            }
                        }
                        CommandKey::SystemFrameStep => {
                            if let Some(nes) = &mut self.nes {
                                nes.pause = true;
                                nes.framestep = true;
                            }
                        }
                        CommandKey::SelectState0 => {
                            self.state_slot = 0;
                        }
                        CommandKey::SelectState1 => {
                            self.state_slot = 1;
                        }
                        CommandKey::SelectState2 => {
                            self.state_slot = 2;
                        }
                        CommandKey::SelectState3 => {
                            self.state_slot = 3;
                        }
                        CommandKey::SelectState4 => {
                            self.state_slot = 4;
                        }
                        CommandKey::SelectState5 => {
                            self.state_slot = 5;
                        }
                        CommandKey::SelectState6 => {
                            self.state_slot = 6;
                        }
                        CommandKey::SelectState7 => {
                            self.state_slot = 7;
                        }
                        CommandKey::SelectState8 => {
                            self.state_slot = 8;
                        }
                        CommandKey::SelectState9 => {
                            self.state_slot = 9;
                        }
                        _ => {}
                    },
                    _ => {}
                }
            }

            if want_save {
                self.save_state();
            }
            if want_load {
                match self.load_state() {
                    Ok(nes) => self.nes = Some(nes),
                    Err(e) => error!("Could not load state {}: {:?}", self.state_slot, e),
                }
            }

            imgui_sdl2.prepare_frame(imgui.io_mut(), &self.window, &self.event_pump.mouse_state());

            let now = Instant::now();
            let delta = now - last_frame;
            imgui.io_mut().delta_time = delta.as_secs_f32();
            last_frame = now;

            let ui = imgui.frame();
            self.draw(&ui);

            glhelper::clear_screen(&self.preferences.background);
            imgui_sdl2.prepare_render(&ui, &self.window);
            renderer.render(ui);
            self.window.gl_swap_window();
            let frame_end = Instant::now();
            let delta = (frame_end - frame_start).as_secs_f64();
            self.preferences.set_frame_time(delta as f32);

            let leftover = (1.0 / Nes::FPS) - delta;
            if leftover > 0.0 {
                //thread::sleep(Duration::from_secs_f64(leftover));
            } else {
                //info!("Timing underrun: {:.03}us", leftover * 1e6);
            }
        }
    }
}
