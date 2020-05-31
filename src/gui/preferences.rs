use imgui;
use imgui::im_str;

#[derive(Clone, Debug)]
pub struct Preferences {
    pub visible: bool,
    pub volume: f32,
    pub scale: f32,
    pub aspect: f32,
    pub background: [f32; 3],
    pub frame_nr: usize,
    pub frame_time: Vec<f32>,
}

impl Preferences {
    pub fn new() -> Self {
        Preferences {
            visible: false,
            volume: 0.5,
            scale: 4.0,
            aspect: 1.0,
            background: [0f32, 0f32, 0.42f32],
            frame_nr: 0,
            frame_time: vec![0f32; 100],
        }
    }

    pub fn set_frame_time(&mut self, time: f32) {
        self.frame_time[self.frame_nr] = time;
        self.frame_nr = (self.frame_nr + 1) % self.frame_time.len();
    }

    pub fn draw(&mut self, ui: &imgui::Ui) {
        if !self.visible {
            return;
        }
        let mut visible =  self.visible;
        let fps: f32 = self.frame_time.iter().sum();
        let fps = self.frame_time.len() as f32 / fps;
        imgui::Window::new(im_str!("Preferences"))
            .opened(&mut visible)
            .build(&ui, || {
                ui.text(
                    format!("Average FPS:       {:>6.02}\nInstantaneous FPS: {:>6.02}",
                            fps, 1.0 / ui.io().delta_time));

                imgui::DragFloat::new(ui, im_str!("Scale"), &mut self.scale)
                    .min(0.0)
                    .max(8.0)
                    .speed(0.01)
                    .display_format(im_str!("%.02f"))
                    .build();

                imgui::DragFloat::new(ui, im_str!("Aspect"), &mut self.aspect)
                    .min(0.0)
                    .max(2.0)
                    .speed(0.01)
                    .display_format(im_str!("%.02f"))
                    .build();

                imgui::Slider::new(im_str!("Volume"), 0.0..=1.0f32)
                    .display_format(im_str!("%.02f"))
                    .build(ui, &mut self.volume);

                imgui::ColorEdit::new(im_str!("Background"),
                                      &mut self.background)
                    .alpha(false)
                    .inputs(false)
                    .picker(true)
                    .build(&ui);
        });
        self.visible = visible;
    }
}
