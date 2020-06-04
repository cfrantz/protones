use super::glhelper::{color_to_f32, f32_to_color};
use imgui;
use imgui::im_str;

pub fn hwpalette_editor(palette: &mut [u32], ui: &imgui::Ui, visible: &mut bool) {
    imgui::Window::new(im_str!("Hardware Palette"))
        .opened(visible)
        .build(ui, || {
            ui.text("Click to edit colors");
            for x in 0..16 {
                if x != 0 {
                    ui.same_line(0.0);
                }
                ui.group(|| {
                    ui.align_text_to_frame_padding();
                    if x == 0 {
                        ui.text("  ");
                        ui.same_line(0.0);
                    }
                    let text = format!("{:02x}", x);
                    ui.text(imgui::ImString::new(text));
                    for y in 0..4 {
                        if x == 0 {
                            let text = format!("{:x}0", y);
                            ui.text(imgui::ImString::new(text));
                            ui.same_line(0.0);
                        }
                        let i = (y * 16 + x) as usize;
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
