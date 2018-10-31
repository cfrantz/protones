#include "nes/apu.h"
#include "nes/cartridge.h"
#include "nes/controller.h"
#include "nes/cpu6502.h"
#include "nes/ppu.h"
#include "nes/mem.h"
#include "nes/nes.h"
#include "pybind11/pybind11.h"
#include "pybind11/embed.h"
#include "pybind11/functional.h"

//#include "version.h"

namespace protones {
namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(protones, m) {
    py::class_<NES, std::shared_ptr<NES> >(m, "NES")
        .def("cpu_cycles", &NES::cpu_cycles, "CPU cycles since reset")
        .def("controller", &NES::controller, "Controller device")
        .def("frame", &NES::frame, "Frames since reset")
        .def("palette", &NES::palette, "Translate a NES color to RGBA")
        .def("Emulate", &NES::EmulateFrame, "Emulate for one CPU instruction")
        .def("EmulateFrame", &NES::EmulateFrame, "Emulate a single frame")
        .def("Reset", &NES::Reset, "Reset the emulation")
        .def("IRQ", &NES::IRQ, "Signal an IRQ to the CPU")
        .def("NMI", &NES::NMI, "Signal an NMI to the CPU")
        .def_property_readonly("mem", &NES::mem, "NES memory")
        .def_property_readonly("cartridge", &NES::cartridge)
        .def_property_readonly("cpu", &NES::cpu)
        .def_property_readonly_static("frequency",
                [](py::object /*self*/){ return NES::frequency; });

    py::class_<Cartridge>(m, "Cartridge")
        .def_property_readonly("mirror", &Cartridge::mirror, "Mirror mode")
        .def_property_readonly("battery", &Cartridge::battery,
                               "Whether this cartridge has a battery")
        .def_property_readonly("mapper", &Cartridge::mapper, "Mapper number")
        .def_property_readonly("prglen", &Cartridge::prglen, "PRG length (bytes)")
        .def_property_readonly("chrlen", &Cartridge::chrlen, "CHR length (bytes)")
        .def("ReadPrg", &Cartridge::ReadPrg, "Read PRG ROM")
        .def("ReadChr", &Cartridge::ReadChr, "Read CHR Rom")
        .def("ReadSram", &Cartridge::ReadSram, "Read SRAM")
        .def("WritePrg", &Cartridge::WritePrg, "Write PRG ROM")
        .def("WriteChr", &Cartridge::WriteChr, "Write CHR Rom")
        .def("WriteSram", &Cartridge::WriteSram, "Write SRAM");

    py::class_<Controller>(m, "Controller")
        .def_property("buttons",
                      &Controller::buttons,
                      (void (Controller::*)(int))&Controller::set_buttons,
                      "Controller button state");

    py::class_<Cpu>(m, "Cpu")
        .def_property("a", &Cpu::a, &Cpu::set_a)
        .def_property("x", &Cpu::x, &Cpu::set_x)
        .def_property("y", &Cpu::y, &Cpu::set_y)
        .def_property("sp", &Cpu::sp, &Cpu::set_sp)
        .def_property("flags", &Cpu::flags, &Cpu::set_flags)
        .def_property("pc", &Cpu::pc, &Cpu::set_pc)
        .def_property_readonly("irq_pending", &Cpu::irq_pending)
        .def("Reset", &Cpu::Reset, "Reset the CPU")
        .def("IRQ", &Cpu::IRQ, "Signal an IRQ to the CPU")
        .def("NMI", &Cpu::NMI, "Signal an NMI to the CPU")
        .def("SetReadCallback", &Cpu::set_read_cb)
        .def("SetWriteCallback", &Cpu::set_write_cb)
        .def("SetExecCallback", &Cpu::set_exec_cb)
        .def("Disassemble", [](Cpu* self, uint16_t addr) {
            std::string s = self->Disassemble(&addr);
            return std::make_pair(addr, s);
        });

    py::class_<Mem>(m, "Memory")
        .def("__getitem__", &Mem::read_byte)
        .def("__setitem__", &Mem::write_byte)
        .def("ReadSignedByte", [](Mem* self, uint16_t addr) -> int8_t {
                return int8_t(self->read_byte(addr));
        }, py::arg("addr"))
        .def("ReadWord", [](Mem* self, uint16_t addrl, uint16_t addrh) -> uint16_t {
                return self->read_byte(addrl) |
                       uint16_t(self->read_byte(addrh)) << 8;
        }, py::arg("addrl"), py::arg("addrh"))
        .def("ReadSignedWord", [](Mem* self, uint16_t addrl, uint16_t addrh) -> int16_t {
                return self->read_byte(addrl) |
                       int16_t(self->read_byte(addrh)) << 8;
        }, py::arg("addrl"), py::arg("addrh"))
        .def("WriteWord", [](Mem* self, uint16_t addrl, uint16_t addrh, uint16_t val) {
                self->write_byte(addrl, val);
                self->write_byte(addrh, val >> 8);
        }, py::arg("addrl"), py::arg("addrh"), py::arg("val"))
        .def("Read", [](Mem* self, uint16_t addr, uint16_t len) {
            std::string v(len, '\0');
            for(uint16_t i=0; i<len; i++) v[i] = self->read_byte(addr+i);
            return py::bytes(v);
        }, py::arg("addr"), py::arg("len"))
        .def("Write", [](Mem* self, uint16_t addr, const std::string& val) {
            for(const char& v : val) {
                self->write_byte(addr++, (uint8_t)v);
            }
        }, py::arg("addr"), py::arg("value_str"));

    m.attr("BUTTON_A") =      0x01; //Controller::BUTTON_A;
    m.attr("BUTTON_B") =      0x02; //Controller::BUTTON_B;
    m.attr("BUTTON_SELECT") = 0x04; //Controller::BUTTON_SELECT;
    m.attr("BUTTON_START") =  0x08; //Controller::BUTTON_START;
    m.attr("BUTTON_UP") =     0x10; //Controller::BUTTON_UP;
    m.attr("BUTTON_DOWN") =   0x20; //Controller::BUTTON_DOWN;
    m.attr("BUTTON_LEFT") =   0x40; //Controller::BUTTON_LEFT;
    m.attr("BUTTON_RIGHT") =  0x80; //Controller::BUTTON_RIGHT;
}
}  // namespace protones
