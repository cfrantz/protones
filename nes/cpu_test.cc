#include <cstdint>
#include <cstdio>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "nes/cpu6502.h"
#include "nes/mem.h"

ABSL_FLAG(int32_t, end, 0, "End address");
ABSL_FLAG(int64_t, max_cycles, 0, "Maxiumum number of cycles to emulate");

class Memory: public protones::Mem {
  public:
    Memory() : Mem(nullptr) {}

    uint8_t read_byte(uint16_t addr) override { return ram_[addr]; }
    void write_byte(uint16_t addr, uint8_t val) override { ram_[addr] = val; }

    uint8_t read_byte_no_io(uint16_t addr) override { return read_byte(addr); }
    void write_byte_no_io(uint16_t addr, uint8_t val) override { write_byte(addr, val); }

    void Load(const std::string& file, uint16_t addr) {
        FILE* fp = fopen(file.c_str(), "rb");
        size_t n = fread(ram_+addr, 1, 65536-addr, fp);
        printf("Read %zu bytes into ram\n", n);
        fclose(fp);
    }
  private:
    uint8_t ram_[64*1024];
};

Memory mem;
protones::Cpu cpu;

int main(int argc, char *argv[]) {
    printf("argc=%d argv=%p\n", argc, argv);
    auto args = absl::ParseCommandLine(argc, argv);

    mem.Load(args[1], 0x400);
    cpu.memory(&mem);
    cpu.set_pc(0x400);


    for(;;) {
        //printf("%04X: %02X %lu\n", cpu.pc(), mem.read_byte(cpu.pc()), cpu.cycles());
        uint16_t pc = cpu.pc();
        std::string instr = cpu.Disassemble(&pc);
        std::string state = cpu.CpuState();
        printf("%s\n   %-40s %lu (0x%lx)\n", state.c_str(), instr.c_str(), cpu.cycles(), cpu.cycles());

        cpu.Emulate();
        if (cpu.pc() == absl::GetFlag(FLAGS_end) ||
            (absl::GetFlag(FLAGS_max_cycles) && cpu.cycles() >= absl::GetFlag(FLAGS_max_cycles))) {
            printf("SUCCESS!\n");
            break;
        }
    }

    return 0;
}
