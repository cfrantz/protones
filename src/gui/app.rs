use std::time::Instant;
use imgui;
use imgui::im_str;
use imgui::MenuItem;
use imgui_sdl2::ImguiSdl2;
use imgui_opengl_renderer::Renderer;
use sdl2;
use sdl2::event::Event;
use sdl2::controller::GameController;
use sdl2::keyboard::Keycode;
use sdl2::audio::{AudioCallback, AudioDevice, AudioSpecDesired};
use nfd;
use log::{info, error};
use std::io;
use std::sync::mpsc;

use crate::nes::nes::Nes;
use crate::gui::glhelper;
use crate::gui::hwpalette::hwpalette_editor;
use crate::gui::input::SdlInput;
use crate::gui::ppu::PpuDebug;
use crate::gui::apu::ApuDebug;

struct Playback {
    queue: mpsc::Receiver<Vec<f32>>,
}

impl AudioCallback for Playback {
    type Channel = f32;
    fn callback(&mut self, output: &mut [f32]) {
        match self.queue.try_recv() {
            Ok(values) => {
                for (sample, value) in output.iter_mut().zip(values) {
                    *sample = value;
                }
            },
            Err(_) => {
                info!("audio underrun!");
                for sample in output.iter_mut() {
                    *sample = 0.0;
                }
            },
        }
    }
}

pub struct App {
    #[allow(dead_code)]  // keep SDL context.
    sdl_context: sdl2::Sdl,
    video: sdl2::VideoSubsystem,
    #[allow(dead_code)]  // keep audio context.
    audio: sdl2::AudioSubsystem,
    #[allow(dead_code)]  // keep audio playback device.
    playback: AudioDevice<Playback>,
    #[allow(dead_code)]  // keep gamecontroller context.
    gamecontoller: sdl2::GameControllerSubsystem,
    window: sdl2::video::Window,
    #[allow(dead_code)]  // keep GL context.
    gl_context: sdl2::video::GLContext,
    event_pump: sdl2::EventPump,
    #[allow(dead_code)]  // keep list of open controllers.
    controllers: Vec<GameController>,
    queue: mpsc::SyncSender<Vec<f32>>,
    background: [f32; 3],
    running: bool,
    palette_editor: bool,
    preferences: bool,

    nes: Option<Box<Nes>>,
    nes_image: imgui::TextureId,
    pub trace: bool,

    ppu_debug: PpuDebug,
    apu_debug: ApuDebug,
}

impl App {
    pub fn new(name: &str, width: u32, height: u32) -> Result<Self, String> {
        let sdl_context = sdl2::init()?;
        let video = sdl_context.video()?;
        {
            let gl_attr = video.gl_attr();
            gl_attr.set_context_profile(sdl2::video::GLProfile::Core);                   
            gl_attr.set_context_version(3, 0);             
        }
        let window = video.window(name, width, height)
            .position_centered()
            .resizable()
            .opengl()
            .allow_highdpi()
            .build().unwrap();

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
        let playback = audio.open_playback(
            None, &want_spec, |_spec| Playback { queue: receiver, })?;
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
            background: [0f32, 0f32, 0.42f32],
            running: true,
            palette_editor: false,
            preferences: false,
            nes: None,
            nes_image: glhelper::new_blank_image(256, 240),
            trace: false,
            ppu_debug: PpuDebug::new(),
            apu_debug: ApuDebug::new(),
        })
    }

    fn open_controllers(gcss: &sdl2::GameControllerSubsystem) -> Result<Vec<GameController>, String> {
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
        let mut nes = Box::new(Nes::from_file(filename)?);
        nes.reset();
        nes.trace = self.trace;
        self.nes = Some(nes);
        Ok(())
    }

    fn loader(&mut self) -> io::Result<()> {
        let result = nfd::open_file_dialog(None, None).unwrap();
        match result {
            nfd::Response::Okay(path) => { self.load(&path) },
            _ => { Ok(()) }
        }
    }

    fn draw_preferences(&mut self, ui: &imgui::Ui) {
        let pref = &mut self.preferences;
        let bg = &mut self.background;
        imgui::Window::new(im_str!("Preferences"))
            .opened(pref)
            .build(&ui, || {
                ui.text(format!("FPS: {:.1}", 1.0 / ui.io().delta_time));
                imgui::ColorEdit::new(im_str!("background"), bg)
                    .alpha(false)
                    .inputs(false)
                    .picker(true)
                    .build(&ui);
        });
    }

    fn draw_nes(&mut self, ui: &imgui::Ui) {
        if let Some(nes) = &mut self.nes {
            nes.emulate_frame();
            let mut audio = nes.audio.borrow_mut();
            if audio.len() >= 1024 {
                let frag: Vec<_> = audio.drain(0..1024).collect();
                self.queue.send(frag).unwrap();
            }
        }
        if let Some(nes) = &self.nes {
            glhelper::update_image(self.nes_image,
                              0, 0, 256, 240, &nes.ppu.borrow().picture);
            imgui::Window::new(im_str!("NES")).build(ui, || {
                imgui::Image::new(self.nes_image, [256.0*4.0, 240.0*4.0]).build(ui);
            });
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
                MenuItem::new(im_str!("Hardware Palette")).build_with_ref(ui, &mut self.palette_editor);
                MenuItem::new(im_str!("Preferences")).build_with_ref(ui, &mut self.preferences);
            });
            ui.menu(im_str!("View"), true, || {
                MenuItem::new(im_str!("Audio Debug")).build_with_ref(ui, &mut self.apu_debug.visible);
                MenuItem::new(im_str!("CHR Viewer")).build_with_ref(ui, &mut self.ppu_debug.chr_visible);
                MenuItem::new(im_str!("VRAM Viewer")).build_with_ref(ui, &mut self.ppu_debug.vram_visible);
            })
        });
        
        if self.preferences {
            self.draw_preferences(ui);
        }
        if let Some(nes) = &self.nes {
            self.apu_debug.draw(nes, ui);
            self.ppu_debug.draw_chr(nes, ui);
            self.ppu_debug.draw_vram(nes, ui);
        }
        if self.palette_editor {
            let visible = &mut self.palette_editor;
            if let Some(nes) = &self.nes {
                let pal = &mut nes.palette.borrow_mut();
                hwpalette_editor(pal, ui, visible);
            }

        }
        self.draw_nes(ui);
    }

    pub fn run(&mut self) {
        let mut last_frame = Instant::now();
        let mut imgui = imgui::Context::create();
        let mut imgui_sdl2 = ImguiSdl2::new(&mut imgui, &self.window);
        let renderer = Renderer::new(
            &mut imgui, |s| self.video.gl_get_proc_address(s) as _);

        'running: while self.running {
            let frame_start = Instant::now();
            for event in self.event_pump.poll_iter() {
                imgui_sdl2.handle_event(&mut imgui, &event);
                if imgui_sdl2.ignore_event(&event) {
                    continue;
                }
                if let Some(nes) = &self.nes {
                    nes.controller[0].borrow_mut().process_event(&event);
                }
                match event {
                    Event::Quit {..} => { break 'running },
                    Event::KeyUp { keycode: Some(Keycode::F11), ..} => {
                        if let Some(nes) = &mut self.nes {
                            nes.reset();
                        }
                    }
                    _ => {}
                }
            }

            imgui_sdl2.prepare_frame(imgui.io_mut(), &self.window,
                                     &self.event_pump.mouse_state());

            let now = Instant::now();
            let delta = now - last_frame;
            imgui.io_mut().delta_time = delta.as_secs_f32();
            last_frame = now;

            let ui = imgui.frame();
            self.draw(&ui);

            glhelper::clear_screen(&self.background);
            imgui_sdl2.prepare_render(&ui, &self.window);
            renderer.render(ui);
            self.window.gl_swap_window();
            let frame_end = Instant::now();
            let delta = (frame_end - frame_start).as_secs_f64();
            let leftover = (1.0/Nes::FPS) - delta;
            if leftover > 0.0 {
                //thread::sleep(Duration::from_secs_f64(leftover));
            } else {
                //info!("Timing underrun: {:.03}us", leftover * 1e6);
            }
        }
    }
}
