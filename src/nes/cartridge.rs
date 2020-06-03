use byteorder::{LittleEndian, ReadBytesExt, WriteBytesExt};
use log::info;
use memmap::MmapMut;
use std::fmt;
use std::fs::File;
use std::fs::OpenOptions;
use std::io;
use std::io::{Error, ErrorKind};
use std::io::{Read, Write};
use std::path::PathBuf;
use std::vec::Vec;

#[derive(Debug, Clone, Default)]
pub struct InesHeader {
    pub signature: u32,
    pub prgsz: u8,
    pub chrsz: u8,
    pub mirror: bool,
    pub battery: bool,
    pub trainer: bool,
    pub fourscreen: bool,
    pub vs_unisystem: bool,
    pub playchoice10: bool,
    pub version: u8,
    pub mapper: u8,
    pub unused: [u8; 8],
}

pub struct SRam {
    pub data: MmapMut,
}

pub struct Cartridge {
    pub header: InesHeader,
    pub prg: Vec<u8>,
    pub chr: Vec<u8>,
    pub sram: SRam,
}

impl fmt::Debug for Cartridge {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Point")
            .field("header", &self.header)
            .finish()
    }
}

impl InesHeader {
    pub fn from_reader(mut r: impl Read) -> io::Result<Self> {
        let signature = r.read_u32::<LittleEndian>()?;
        if signature != 0x1a53454eu32 {
            return Err(Error::new(ErrorKind::Other, "Invalid iNES signature"));
        }
        let prgsz = r.read_u8()?;
        let chrsz = r.read_u8()?;
        let flags6 = r.read_u8()?;
        let flags7 = r.read_u8()?;
        let mut unused = [0; 8];
        r.read_exact(&mut unused)?;

        Ok(InesHeader {
            signature: signature,
            prgsz: prgsz,
            chrsz: chrsz,
            mirror: (flags6 & 0x01) != 0,
            battery: (flags6 & 0x02) != 0,
            trainer: (flags6 & 0x04) != 0,
            fourscreen: (flags6 & 0x08) != 0,
            vs_unisystem: (flags7 & 0x01) != 0,
            playchoice10: (flags7 & 0x02) != 0,
            version: (flags7 >> 2) & 0x03,
            mapper: (flags6 >> 4 | flags7 & 0xF0),
            unused: unused,
        })
    }

    pub fn write(&self, w: &mut impl Write) -> io::Result<()> {
        w.write_u32::<LittleEndian>(self.signature)?;
        w.write_u8(self.prgsz)?;
        w.write_u8(self.chrsz)?;
        let flags6 = (self.mapper << 4)
            | if self.mirror { 0x01 } else { 0x00 }
            | if self.battery { 0x02 } else { 0x00 }
            | if self.trainer { 0x04 } else { 0x00 }
            | if self.fourscreen { 0x08 } else { 0x00 };
        let flags7 = (self.mapper & 0xF0)
            | (self.version << 2)
            | if self.vs_unisystem { 0x01 } else { 0x00 }
            | if self.playchoice10 { 0x02 } else { 0x00 };
        w.write_u8(flags6)?;
        w.write_u8(flags7)?;
        w.write_all(&self.unused)?;
        Ok(())
    }
}

impl SRam {
    pub fn from_file(filepath: &PathBuf, length: usize) -> io::Result<Self> {
        let sramfile = OpenOptions::new()
            .read(true)
            .write(true)
            .create(true)
            .open(filepath)?;

        sramfile.set_len(length as u64)?;
        Ok(SRam {
            data: unsafe { MmapMut::map_mut(&sramfile)? },
        })
    }
    pub fn anon(length: usize) -> io::Result<Self> {
        Ok(SRam {
            // Can't create a zero length mapping.
            data: MmapMut::map_anon(length)?,
        })
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
}

impl Cartridge {
    pub fn from_reader(mut r: impl Read, savefile: Option<PathBuf>) -> io::Result<Self> {
        let mut header = InesHeader::from_reader(&mut r)?;
        let mut prg = vec![0; header.prgsz as usize * 16384];
        r.read_exact(&mut prg)?;

        let load_chr = header.chrsz != 0;
        if !load_chr {
            header.chrsz = 1;
        }
        let mut chr = vec![0; header.chrsz as usize * 8192];
        if load_chr {
            r.read_exact(&mut chr)?;
        }

        // FIXME: different mappers generally represent different amounts of
        // sram in the cartridge.  Generally, carts provide 8K.  MMC5 can
        // provide upto 128K, but it seems common for it to provide between
        // 8K and 64K.
        let sramsize = 8192;
        let sram = if header.battery {
            if let Some(savefile) = savefile {
                SRam::from_file(&savefile, sramsize)?
            } else {
                // Have SRAM, but no filename, so anonymous mapping.
                SRam::anon(sramsize)?
            }
        } else {
            SRam::anon(sramsize)?
        };

        info!("iNES header = {:?}", header);
        Ok(Cartridge {
            header,
            prg,
            chr,
            sram,
        })
    }

    pub fn from_file(filename: &str, savefile: Option<PathBuf>) -> io::Result<Self> {
        let file = File::open(filename)?;
        Cartridge::from_reader(file, savefile)
    }

    pub fn write(&self, w: &mut impl Write) -> io::Result<()> {
        self.header.write(w)?;
        w.write_all(&self.prg)?;
        w.write_all(&self.chr)?;
        Ok(())
    }

    pub fn save(&self, filename: &str) -> io::Result<()> {
        let mut file = File::create(filename)?;
        self.write(&mut file)?;
        Ok(())
    }
    pub fn mirror_address(&self, address: u16) -> u16 {
        let address = address & 0xFFF;
        if self.header.fourscreen {
            address
        } else if self.header.mirror {
            // Vertical mirroring
            address & !0x800
        } else {
            // Horizontal mirroring
            let a11 = address & 0x800;
            (address & !0xc00) | (a11 >> 1)
        }
    }
}
