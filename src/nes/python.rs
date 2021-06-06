use crate::nes::cpu6502::{CpuFlags, Memory};
use crate::nes::nes::Nes;
use pyo3::class::PySequenceProtocol;

//use crate::nes::apu::Apu;
//use crate::nes::cartridge::Cartridge;
//use crate::nes::controller::Controller;
//use crate::nes::mapper;
//use crate::nes::mapper::Mapper;
//use crate::nes::ppu::Ppu;

use pyo3::prelude::*;

#[pyclass(unsendable)]
pub struct Cpu6502 {
    pub nes: Py<Nes>,
}

#[pymethods]
impl Cpu6502 {
    #[getter]
    fn get_a(&self, py: Python) -> u8 {
        self.nes.borrow(py).cpu.borrow().a
    }
    #[setter]
    fn set_a(&self, py: Python, val: u8) {
        self.nes.borrow(py).cpu.borrow_mut().a = val;
    }

    #[getter]
    fn get_x(&self, py: Python) -> u8 {
        self.nes.borrow(py).cpu.borrow().x
    }
    #[setter]
    fn set_x(&self, py: Python, val: u8) {
        self.nes.borrow(py).cpu.borrow_mut().x = val;
    }

    #[getter]
    fn get_y(&self, py: Python) -> u8 {
        self.nes.borrow(py).cpu.borrow().y
    }
    #[setter]
    fn set_y(&self, py: Python, val: u8) {
        self.nes.borrow(py).cpu.borrow_mut().y = val;
    }

    #[getter]
    fn get_flags(&self, py: Python) -> u8 {
        self.nes.borrow(py).cpu.borrow().p.bits()
    }
    #[setter]
    fn set_flags(&self, py: Python, val: u8) {
        self.nes.borrow(py).cpu.borrow_mut().p = CpuFlags::from_bits_truncate(val);
    }

    #[getter]
    fn get_pc(&self, py: Python) -> u16 {
        self.nes.borrow(py).cpu.borrow().pc
    }
    #[setter]
    fn set_pc(&self, py: Python, val: u16) {
        self.nes.borrow(py).cpu.borrow_mut().pc = val;
    }

    #[getter]
    fn get_sp(&self, py: Python) -> u16 {
        0x100 | (self.nes.borrow(py).cpu.borrow().sp as u16)
    }
    #[setter]
    fn set_sp(&self, py: Python, val: u16) {
        self.nes.borrow(py).cpu.borrow_mut().sp = val as u8;
    }

    #[getter]
    fn get_reset_pending(&self, py: Python) -> bool {
        self.nes.borrow(py).cpu.borrow().reset_pending
    }
    #[setter]
    fn set_reset_pending(&self, py: Python, val: bool) {
        self.nes.borrow(py).cpu.borrow_mut().reset_pending = val;
    }

    #[getter]
    fn get_nmi_pending(&self, py: Python) -> bool {
        self.nes.borrow(py).cpu.borrow().nmi_pending
    }
    #[setter]
    fn set_nmi_pending(&self, py: Python, val: bool) {
        self.nes.borrow(py).cpu.borrow_mut().nmi_pending = val;
    }

    #[getter]
    fn get_irq_pending(&self, py: Python) -> bool {
        self.nes.borrow(py).cpu.borrow().irq_pending
    }
    #[setter]
    fn set_irq_pending(&self, py: Python, val: bool) {
        self.nes.borrow(py).cpu.borrow_mut().irq_pending = val;
    }
}

#[pyclass(unsendable)]
pub struct Mem {
    pub nes: Py<Nes>,
}

#[pymethods]
impl Mem {
    fn read(&self, py: Python, address: u16) -> u8 {
        self.nes.borrow(py).read(address)
    }
    fn write(&self, py: Python, address: u16, val: u8) {
        self.nes.borrow(py).write(address, val)
    }

    fn read_word(&self, py: Python, addr_lo: u16, addr_hi: u16) -> u16 {
        let mem = self.nes.borrow(py);
        (mem.read(addr_lo) as u16) | ((mem.read(addr_hi) as u16) << 8)
    }
    fn write_word(&self, py: Python, addr_lo: u16, addr_hi: u16, val: u16) {
        let mem = self.nes.borrow(py);
        mem.write(addr_lo, val as u8);
        mem.write(addr_hi, (val >> 8) as u8);
    }
}

#[pyproto]
impl PySequenceProtocol for Mem {
    fn __getitem__(&self, address: isize) -> u8 {
        Python::with_gil(|py| self.read(py, address as u16))
    }
    fn __setitem__(&mut self, address: isize, val: u8) {
        Python::with_gil(|py| self.write(py, address as u16, val));
    }
}
