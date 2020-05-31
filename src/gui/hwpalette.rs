use imgui;
use imgui::im_str;

const R_SHIFT: usize = 0;
const G_SHIFT: usize = 8;
const B_SHIFT: usize = 16;
const A_SHIFT: usize = 24;

fn color_to_f32(color: u32, fcol: &mut [f32; 4]) {
    fcol[0] = ((color >> R_SHIFT) & 0xFF) as f32 / 255.0;
    fcol[1] = ((color >> G_SHIFT) & 0xFF) as f32 / 255.0;
    fcol[2] = ((color >> B_SHIFT) & 0xFF) as f32 / 255.0;
    fcol[3] = ((color >> A_SHIFT) & 0xFF) as f32 / 255.0;
}

fn f32_to_color(fcol: &[f32; 4]) -> u32 {
    let r = (fcol[0] * 255.0) as u32;
    let g = (fcol[1] * 255.0) as u32;
    let b = (fcol[2] * 255.0) as u32;
    let a = (fcol[3] * 255.0) as u32;
    (r << R_SHIFT) | (g << G_SHIFT) | (b << B_SHIFT) | (a << A_SHIFT)
}


pub fn hwpalette_editor(palette: &mut [u32], ui: &imgui::Ui, visible: &mut bool) {
    imgui::Window::new(im_str!("Hardware Palette"))
        .opened(visible)
        .build(ui, || {
            ui.text("Click to edit colors");
            for x in 0..16 {
                if x != 0 { ui.same_line(0.0); }
                ui.group(|| {
                    ui.align_text_to_frame_padding();
                    if x == 0 {
                        ui.text("  "); ui.same_line(0.0);
                    }
                    let text = format!("{:02x}", x);
                    ui.text(imgui::ImString::new(text));
                    for y in 0..4 {
                        if x == 0 {
                            let text = format!("{:x}0", y);
                            ui.text(imgui::ImString::new(text));
                            ui.same_line(0.0);
                        }
                        let i = (y*16+x) as usize;
                        let id = ui.push_id(i as i32);
                        let mut fcol = [0.0f32; 4];
                        color_to_f32(palette[i], &mut fcol);
                        if imgui::ColorButton::new(im_str!(""), fcol).build(ui) {
                            ui.open_popup(im_str!("edit"));
                        }
                        ui.popup(im_str!("edit"), || {
                            let cp = imgui::ColorPicker::new(im_str!("color"), &mut fcol);
                            if cp.build(&ui) {
                                palette[i] = f32_to_color(&fcol);
                            }
                            if ui.small_button(im_str!("Close")) {
                                ui.close_current_popup();
                            }
                        });
                        id.pop(ui);
                    }
                });
            }
        });
}
