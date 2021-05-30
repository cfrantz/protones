use imgui;
use imgui::im_str;

use crate::nes::apu_dmc::ApuDmc;
use crate::nes::apu_noise::ApuNoise;
use crate::nes::apu_pulse::ApuPulse;
use crate::nes::apu_triangle::ApuTriangle;
use crate::nes::nes::Nes;

#[derive(Clone, Debug)]
pub struct ApuDebug {
    pub visible: bool,
}

impl ApuDebug {
    pub fn new() -> Self {
        ApuDebug { visible: false }
    }

    fn draw_pulse(ui: &imgui::Ui, pulse: &mut ApuPulse) {
        let name = imgui::ImString::new(format!("Pulse {}", pulse.channel));
        ui.plot_lines(im_str!(""), &pulse.dbg_buf)
            .overlay_text(&name)
            .values_offset(pulse.dbg_offset)
            .scale_min(0.0)
            .scale_max(1.0)
            .graph_size([0.0, 64.0])
            .build();
        ui.same_line(0.0);
        ui.group(|| {
            ui.text(format!("Control: {:02x}", pulse.reg.control));
            ui.text(format!("Sweep:   {:02x}", pulse.reg.sweep));
            ui.text(format!(
                "Timer:   {:02x}{:02x}",
                pulse.reg.timer_hi, pulse.reg.timer_lo
            ));
        });
        imgui::Slider::new(&name)
            .range(0.0..=1.0f32)
            .display_format(im_str!("%.02f"))
            .build(ui, &mut pulse.output_volume);
    }

    fn draw_triangle(ui: &imgui::Ui, triangle: &mut ApuTriangle) {
        ui.plot_lines(im_str!(""), &triangle.dbg_buf)
            .overlay_text(im_str!("Triangle"))
            .values_offset(triangle.dbg_offset)
            .scale_min(0.0)
            .scale_max(1.0)
            .graph_size([0.0, 64.0])
            .build();
        ui.same_line(0.0);
        ui.group(|| {
            ui.text(format!("Control: {:02x}", triangle.reg.control));
            ui.text(format!(
                "Timer:   {:02x}{:02x}",
                triangle.reg.timer_hi, triangle.reg.timer_lo
            ));
        });
        imgui::Slider::new(im_str!("Triangle"))
            .range(0.0..=1.0f32)
            .display_format(im_str!("%.02f"))
            .build(ui, &mut triangle.output_volume);
    }

    fn draw_noise(ui: &imgui::Ui, noise: &mut ApuNoise) {
        ui.plot_lines(im_str!(""), &noise.dbg_buf)
            .overlay_text(im_str!("Noise"))
            .values_offset(noise.dbg_offset)
            .scale_min(0.0)
            .scale_max(1.0)
            .graph_size([0.0, 64.0])
            .build();
        ui.same_line(0.0);
        ui.group(|| {
            ui.text(format!("Control: {:02x}", noise.reg.control));
            ui.text(format!("Period:  {:02x}", noise.reg.period));
            ui.text(format!("Length:  {:02x}", noise.reg.length));
        });
        imgui::Slider::new(im_str!("Noise"))
            .range(0.0..=1.0f32)
            .display_format(im_str!("%.02f"))
            .build(ui, &mut noise.output_volume);
    }

    fn draw_dmc(ui: &imgui::Ui, dmc: &mut ApuDmc) {
        ui.plot_lines(im_str!(""), &dmc.dbg_buf)
            .overlay_text(im_str!("DMC"))
            .values_offset(dmc.dbg_offset)
            .scale_min(0.0)
            .scale_max(1.0)
            .graph_size([0.0, 64.0])
            .build();
        ui.same_line(0.0);
        ui.group(|| {
            ui.text(format!("Control: {:02x}", dmc.reg.control));
            ui.text(format!("Value:   {:02x}", dmc.reg.value));
            ui.text(format!("Address: {:02x}", dmc.reg.address));
            ui.text(format!("Length:  {:02x}", dmc.reg.length));
        });
        imgui::Slider::new(im_str!("DMC"))
            .range(0.0..=1.0f32)
            .display_format(im_str!("%.02f"))
            .build(ui, &mut dmc.output_volume);
    }

    pub fn draw(&mut self, nes: &Nes, ui: &imgui::Ui) {
        if !self.visible {
            return;
        }
        let mut visible = self.visible;
        imgui::Window::new(im_str!("Audio"))
            .opened(&mut visible)
            .build(&ui, || {
                let mut apu = nes.apu.borrow_mut();
                ApuDebug::draw_pulse(ui, &mut apu.pulse0);
                ApuDebug::draw_pulse(ui, &mut apu.pulse1);
                ApuDebug::draw_triangle(ui, &mut apu.triangle);
                ApuDebug::draw_noise(ui, &mut apu.noise);
                ApuDebug::draw_dmc(ui, &mut apu.dmc);
            });
        self.visible = visible;
    }
}
