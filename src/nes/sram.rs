use serde::{Deserialize, Serialize};
use std::fs::File;
use std::io;
use std::io::{Read, Write};
use std::path::PathBuf;
use std::vec::Vec;

#[derive(Default, Debug, Serialize, Deserialize)]
pub struct SRam {
    pub data: Vec<u8>,
}

impl SRam {
    pub fn with_size(length: usize) -> Self {
        SRam {
            data: vec![0u8; length],
        }
    }

    pub fn read(&self, offset: usize) -> u8 {
        if offset < self.data.len() {
            self.data[offset]
        } else {
            error!("SRAM: bad read at {:04x}", offset);
            0xff
        }
    }

    pub fn write(&mut self, offset: usize, value: u8) {
        if offset < self.data.len() {
            self.data[offset] = value;
        } else {
            error!("SRAM: bad write at {:04x}, value={:02x}", offset, value);
        }
    }

    pub fn load(&mut self, filepath: &PathBuf) -> io::Result<()> {
        let mut file = File::open(filepath)?;
        file.read_exact(&mut self.data)
    }

    pub fn save(&self, filepath: &PathBuf) -> io::Result<()> {
        let mut file = File::create(filepath)?;
        file.write_all(&self.data)
    }
}
