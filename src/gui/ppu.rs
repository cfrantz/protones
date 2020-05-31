use imgui;
use imgui::im_str;

use crate::gui::glhelper;
use crate::nes::nes::Nes;

#[derive(Clone, Debug)]
pub struct PpuDebug {
    pub chr_visible: bool,
    pub vram_visible: bool,
    chr_pixels: Vec<u32>,
    chr_image: [imgui::TextureId; 2],
}

const HEXDIGIT: [u8; 16] = [
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46,
];

impl PpuDebug {
    pub fn new() -> Self {
        PpuDebug {
            chr_visible: false,
            vram_visible: false,
            chr_pixels: vec![0u32; 128*128],
            chr_image: [
                glhelper::new_blank_image(128, 128),
                glhelper::new_blank_image(128, 128),
            ],
        }
    }
    fn update_chr_image(&mut self, nes: &Nes, bank: usize) {
        let pal = [0xFF000000u32, 0xFF666666u32, 0xFFAAAAAAu32, 0xFFFFFFFFu32];
        let offset = bank * 0x1000;
        for y in 0..16 {
            for x in 0..16 {
                let tile = y * 16 + x;
                for row in 0..8 {
                    let mut a = nes.ppu_read((offset+16*tile+row) as u16);
                    let mut b = nes.ppu_read((offset+16*tile+row+8) as u16);
                    for col in 0..8 {
                        let color = ((a & 0x80) >> 7) | ((b & 0x80) >> 6);
                        self.chr_pixels[128*(8*y+row) + 8*x+col] = pal[color as usize];
                        a <<= 1; b <<= 1;
                    }
                }
            }
        }
        glhelper::update_image(self.chr_image[bank],
                               0, 0, 128, 128,
                               &self.chr_pixels);
    }

    pub fn draw_chr(&mut self, nes: &Nes, ui: &imgui::Ui) {
        if !self.chr_visible {
            return;
        }
        let mut visible =  self.chr_visible;
        imgui::Window::new(im_str!("CHR View"))
            .opened(&mut visible)
            .build(&ui, || {
                for i in 0..2 {
                    self.update_chr_image(nes, i);
                    ui.text(format!("Pattern Table {}", i));
                    imgui::Image::new(self.chr_image[i],
                                      [128.0*4.0, 128.0*4.0]).build(ui);
                }
            });
        self.chr_visible = visible;
    }

    fn hexdump_vram(&mut self, nes: &Nes, ui: &imgui::Ui, v: u16) {
        let mut v = v;
        for _ in 0..30 {
            let mut line = vec![0u8; 32*2 + 6 + 1];
            for x in 0..32 {
                //let val = nes.ppu_read(v);
                let val = nes.vram.borrow()[(v & 0xFFF) as usize];
                //let a = 0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 7);
                if x == 0 {
                    line[0] = HEXDIGIT[((v >> 12) & 0xF) as usize];
                    line[1] = HEXDIGIT[((v >> 8) & 0xF) as usize];
                    line[2] = HEXDIGIT[((v >> 4) & 0xF) as usize];
                    line[3] = HEXDIGIT[((v >> 0) & 0xF) as usize];
                    line[4] = ':' as u8;
                    line[5] = ' ' as u8;
                }
                line[6 + x*2] = HEXDIGIT[((val >> 4) & 0xF) as usize];
                line[7 + x*2] = HEXDIGIT[((val >> 0) & 0xF) as usize];
                v += 1;
            }
            unsafe {
                ui.text(imgui::ImString::from_utf8_with_nul_unchecked(line));
            }
        }
    }

    pub fn draw_vram(&mut self, nes: &Nes, ui: &imgui::Ui) {
        if !self.vram_visible {
            return;
        }
        let mut visible =  self.vram_visible;
        imgui::Window::new(im_str!("VRAM View"))
            .opened(&mut visible)
            .build(&ui, || {
                ui.group(|| self.hexdump_vram(nes, ui, 0x2000));
                ui.same_line(0.0);
                ui.group(|| self.hexdump_vram(nes, ui, 0x2400));
                ui.text("");
                ui.group(|| self.hexdump_vram(nes, ui, 0x2800));
                ui.same_line(0.0);
                ui.group(|| self.hexdump_vram(nes, ui, 0x2c00));
            });
        self.vram_visible = visible;
    }
}
