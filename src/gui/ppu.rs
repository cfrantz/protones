use imgui;
use imgui::im_str;

use crate::gui::glhelper;
use crate::gui::glhelper::color_as_f32;
use crate::nes::nes::Nes;
use crate::nes::ppu;

#[derive(Clone, Debug)]
pub struct PpuDebug {
    pub chr_visible: bool,
    pub vram_visible: bool,
    chr_pixels: Vec<u32>,
    chr_palette: [Option<u8>; 2],
    chr_image: [imgui::TextureId; 2],
    best_color: [Vec<u8>; 2],
}

const HEXDIGIT: [u8; 16] = [
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,
];

const PALETTE_NAMES: [&str; 8] = [
    "Background 0",
    "Background 1",
    "Background 2",
    "Background 3",
    "    Sprite 0",
    "    Sprite 1",
    "    Sprite 2",
    "    Sprite 3",
];

impl PpuDebug {
    pub fn new() -> Self {
        PpuDebug {
            chr_visible: false,
            vram_visible: false,
            chr_pixels: vec![0u32; 128 * 128],
            chr_palette: [None; 2],
            chr_image: [
                glhelper::new_blank_image(128, 128),
                glhelper::new_blank_image(128, 128),
            ],
            best_color: [vec![0; 256], vec![0; 256]],
        }
    }
    fn most_common(counts: &[u8; 4]) -> u8 {
        let mut max = 0;
        let mut n = 0;
        for (i, count) in counts[1..].iter().enumerate() {
            if *count > max {
                max = *count;
                n = (i + 1) as u8;
            }
        }
        n
    }
    fn update_chr_image(&mut self, nes: &Nes, bank: usize, palette: Option<u8>) {
        let pal = if let Some(p) = palette {
            [
                nes.palette_lookup(p * 4 + 0),
                nes.palette_lookup(p * 4 + 1),
                nes.palette_lookup(p * 4 + 2),
                nes.palette_lookup(p * 4 + 3),
            ]
        } else {
            [0xFF000000u32, 0xFF666666u32, 0xFFAAAAAAu32, 0xFFFFFFFFu32]
        };
        let offset = bank * 0x1000;
        for y in 0..16 {
            for x in 0..16 {
                let tile = y * 16 + x;
                let mut counts = [0u8; 4];
                for row in 0..8 {
                    let mut a = nes.ppu_read((offset + 16 * tile + row) as u16);
                    let mut b = nes.ppu_read((offset + 16 * tile + row + 8) as u16);
                    for col in 0..8 {
                        let color = ((a & 0x80) >> 7) | ((b & 0x80) >> 6);
                        self.chr_pixels[128 * (8 * y + row) + 8 * x + col] = pal[color as usize];
                        counts[color as usize] += 1;
                        a <<= 1;
                        b <<= 1;
                    }
                }
                self.best_color[bank][tile] = PpuDebug::most_common(&counts);
            }
        }
        glhelper::update_image(self.chr_image[bank], 0, 0, 128, 128, &self.chr_pixels);
    }

    fn palette_buttons(palette: u8, nes: &Nes, ui: &imgui::Ui) {
        for i in 0..4 {
            let pcolor = nes.pram.borrow()[(palette * 4 + i) as usize];
            let label = imgui::ImString::new(format!("{:02x}", pcolor));
            let color = nes.palette_lookup(palette * 4 + i);
            ui.same_line(0.0);
            imgui::ColorButton::new(&label, color_as_f32(color)).build(ui);
        }
    }

    fn draw_chr(&mut self, nes: &Nes, ui: &imgui::Ui) {
        if !self.chr_visible {
            return;
        }
        let mut visible = self.chr_visible;
        imgui::Window::new(im_str!("CHR View"))
            .opened(&mut visible)
            .build(&ui, || {
                for i in 0..2 {
                    let id = ui.push_id(imgui::Id::Int(i as i32));
                    ui.text(format!("Pattern Table {}", i));
                    imgui::Image::new(self.chr_image[i], [128.0 * 4.0, 128.0 * 4.0]).build(ui);
                    ui.same_line(0.0);
                    ui.group(|| {
                        ui.radio_button(im_str!("None"), &mut self.chr_palette[i], None);
                        for j in 0..8u8 {
                            let name = imgui::ImString::new(PALETTE_NAMES[j as usize]);
                            ui.radio_button(&name, &mut self.chr_palette[i], Some(j));
                            PpuDebug::palette_buttons(j, nes, ui);
                        }
                    });
                    id.pop(ui);
                }
            });
        self.chr_visible = visible;
    }

    fn print_address(addr: u16, ui: &imgui::Ui) {
        let addr = addr as usize;
        let text = [
            ' ' as u8,
            HEXDIGIT[((addr >> 12) & 0xF)],
            HEXDIGIT[((addr >> 8) & 0xF)],
            HEXDIGIT[((addr >> 4) & 0xF)],
            HEXDIGIT[((addr >> 0) & 0xF)],
            ':' as u8,
            0,
        ];

        unsafe {
            ui.text(imgui::ImStr::from_utf8_with_nul_unchecked(&text));
        }
    }

    fn print_colored_hex(&self, nes: &Nes, table: usize, val: u8, palette: u8, ui: &imgui::Ui) {
        let val = val as usize;
        let text = [
            HEXDIGIT[((val >> 4) & 0xF)],
            HEXDIGIT[((val >> 0) & 0xF)],
            0u8,
        ];
        let color = nes.palette_lookup(palette * 4 + self.best_color[table][val as usize]);
        unsafe {
            ui.text_colored(
                color_as_f32(color),
                imgui::ImStr::from_utf8_with_nul_unchecked(&text),
            );
        }
    }

    fn hexdump_vram(&mut self, nes: &Nes, ui: &imgui::Ui, v: u16) {
        let table = if (nes.ppu.borrow().control & ppu::CTRL_BGTABLE) != 0 {
            1
        } else {
            0
        };
        //let x0 = if (v & 0x400) == 0 { 0 } else {32};
        //let y0 = if (v & 0x800) == 0 { 0 } else {30};
        let mut v = v;
        for _y in 0..30 {
            for x in 0..32 {
                let val = nes.ppu_read(v);
                let a = 0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 7);
                let shift = ((v >> 4) & 4) | (v & 2);
                let attr = (nes.ppu_read(a) >> shift) & 3;
                if x == 0 {
                    PpuDebug::print_address(v, ui);
                }
                ui.same_line(0.0);
                self.print_colored_hex(nes, table, val, attr, ui);
                v += 1;
            }
        }
    }

    fn draw_vram(&mut self, nes: &Nes, ui: &imgui::Ui) {
        if !self.vram_visible {
            return;
        }
        let mut visible = self.vram_visible;
        imgui::Window::new(im_str!("VRAM View"))
            .opened(&mut visible)
            .build(&ui, || {
                let style = ui.push_style_vars(&[imgui::StyleVar::ItemSpacing([1.0, 1.0])]);
                ui.group(|| self.hexdump_vram(nes, ui, 0x2000));
                ui.same_line(0.0);
                ui.group(|| self.hexdump_vram(nes, ui, 0x2400));
                ui.text("");
                ui.group(|| self.hexdump_vram(nes, ui, 0x2800));
                ui.same_line(0.0);
                ui.group(|| self.hexdump_vram(nes, ui, 0x2c00));
                style.pop(&ui);
            });
        self.vram_visible = visible;
    }

    pub fn draw(&mut self, nes: &Nes, ui: &imgui::Ui) {
        if self.chr_visible || self.vram_visible {
            for i in 0..2 {
                let p = self.chr_palette[i];
                self.update_chr_image(nes, i, p);
            }
            self.draw_chr(nes, ui);
            self.draw_vram(nes, ui);
        }
    }
}
