#include <cstdio>
#include <cstdint>
#include <inttypes.h>
#include <gflags/gflags.h>
#include "nes/cpu6502.h"
#include "nes/pbmacro.h"

DEFINE_bool(trace, false, "Enable per cycle CPU tracing");
namespace protones {

void Cpu::Branch(uint16_t addr) {
    static uint16_t last_pc, last_addr;
    if (pc_ == last_pc && addr == last_addr && addr == pc_ - 2)
        abort();
    last_pc = pc_; last_addr = addr;
    if (PagesDiffer(pc_, addr))
        cycles_++;
    pc_ = addr;
    cycles_++;
}


Cpu::Cpu(Mem* mem) :
    mem_(mem),
    flags_{0x24},
    pc_(0),
    sp_(0xFD),
    a_(0), x_(0), y_(0),
    cycles_(0),
    stall_(0),
    nmi_pending_(false),
    irq_pending_(false),
    tbptr_(0),
    halted_(false) {}

void Cpu::SaveState(proto::CPU6502 *state) {
    state->set_flags(flags_.value);
    SAVE(pc, sp, a, x, y, cycles, stall, nmi_pending, irq_pending);
}

void Cpu::LoadState(proto::CPU6502 *state) {
    flags_.value = state->flags();
    LOAD(pc, sp, a, x, y, cycles, stall, nmi_pending, irq_pending);
}

void Cpu::Reset() {
    pc_ = Read16(0xFFFC);
    sp_ = 0xFD;
//    flags_.value = 0x24;
    flags_.value = 0x04;
    a_ = x_ = y_ = 0;
    nmi_pending_ = false;
    irq_pending_ = false;
    cycles_ = 0;
    stall_ = 0;
    Emit("RESET");
}

std::string Cpu::CpuState() {
    char buf[80];
    sprintf(buf, "PC=%04x A=%02x X=%02x Y=%02x SP=1%02x %c%c%c%c%c%c%c%c",
            pc_, a_, x_, y_, sp_,
            flags_.n ? 'N' : 'n',
            flags_.v ? 'V' : 'v',
            flags_.u ? 'U' : 'u',
            flags_.b ? 'B' : 'b',
            flags_.d ? 'D' : 'd',
            flags_.i ? 'I' : 'i',
            flags_.z ? 'Z' : 'z',
            flags_.c ? 'C' : 'c');
    return std::string(buf);
}

std::string Cpu::Disassemble(uint16_t* nexti, bool tracemode) {
    char buf[80];
    char *b = buf;
    uint16_t data = 0;
    uint16_t pc = pc_;

    if (nexti && *nexti)
        pc = *nexti;

    uint8_t opcode = Read(pc);
    InstructionInfo info = info_[opcode];
    int i;

    if (tracemode) {
        buf[0] = ' ';
        buf[1] = ' ';
        buf[2] = ' ';
        buf[3] = ' ';
        b = buf+4;
    }

    switch(info.size) {
    case 0:
        // Illegal opcode
        sprintf(b, "%02x: %02x            %s",
                pc, opcode, instruction_names_[opcode]);
        pc++;
        break;
    case 1:
        sprintf(b, "%02x: %02x            %s",
                pc, opcode, instruction_names_[opcode]);
        break;
    case 2:
        data = Read(pc+1);
        i = sprintf(b, "%02x: %02x%02x          ", pc, opcode, data);
        sprintf(b+i, instruction_names_[opcode], data);
        break;
    case 3:
        data = Read(pc+1) | Read(pc+2)<<8;
        i = sprintf(b, "%02x: %02x%02x%02x        ",
                    pc, opcode, Read(pc+1), Read(pc+2));
        sprintf(b+i, instruction_names_[opcode], data);
        break;
    }
    if (nexti) {
        *nexti = pc + info.size;
    }

    if (tracemode) {
        Emit(buf, 1);
        return "";
    }
    return std::string(buf);
}

void Cpu::Flush() {
    for(int i=tbptr_+1; i != tbptr_; i=(i+1)%TRACEBUFSZ) {
        fputs(tracebuf_[i], stderr);
    }
}

void Cpu::Emit(const char *buf, int how) {
    static uint64_t last_ts;
    uint64_t ts = cycles_;
    if (!FLAGS_trace)
        return;
    if (how < 0) {
        int n = -how;
        sprintf(tracebuf_[tbptr_], "%*s  %s\n", n, "", buf);
    } else if (how == 1) {
        uint64_t dt = ts - last_ts;
        sprintf(tracebuf_[tbptr_], "%" PRIu64 ": %s\n", dt, buf);
        last_ts = ts;
    } else {
        sprintf(tracebuf_[tbptr_], "%" PRIu64 ": %s\n", ts, buf);
        last_ts = ts;
    }
    tbptr_ = (tbptr_ + 1) % TRACEBUFSZ;
//    if (tbptr_ > TRACEBUFSZ) {
//        Flush();
//    }
}



void Cpu::Trace() {
    if (!FLAGS_trace)
        return;
    std::string s = CpuState();
    Emit(s.c_str(), -8);
    Disassemble(nullptr, true);
}

int Cpu::Execute(void) {
    if (halted_)
        return 1;
    if (stall_ > 0) {
      stall_--;
      return 1;
    }
    int cycles = cycles_;

    // Interrupt?
    if (nmi_pending_) {
//        printf("NMI @ %d\n", cycles_);
        nmi_pending_ = false;
        Push16(pc_);
        Push(flags_.value | 0x10);
        pc_ = Read16(0xFFFA);
        flags_.i = true;
        cycles_ += 7;
    } else if (irq_pending_ && !flags_.i) {
        irq_pending_ = false;
        Push16(pc_);
        Push(flags_.value | 0x10);
        pc_ = Read16(0xFFFE);
        flags_.i = true;
        cycles_ += 7;
    }

    // Scratch values
    uint8_t val, a, b;
    int16_t r;

    uint16_t fetchpc = pc_;
    uint16_t addr = 0;
    uint8_t opcode = Read(pc_);
    InstructionInfo info = info_[opcode];
    Trace();
    //exec_cb_(this, pc_, opcode);

#undef TESTCPU
#ifdef TESTCPU
    printf("      A=%02x X=%02x Y=%02x SP=1%02x %c%c%c%c%c%c%c%c\n",
            a_, x_, y_, sp_,
            flags_.n ? 'N' : 'n',
            flags_.v ? 'V' : 'v',
            flags_.u ? 'U' : 'u',
            flags_.b ? 'B' : 'b',
            flags_.d ? 'D' : 'd',
            flags_.i ? 'I' : 'i',
            flags_.z ? 'Z' : 'z',
            flags_.c ? 'C' : 'c');
    switch(info.size) {
    case 1:
        printf("%02x: %02x\n", pc_, opcode);
        break;
    case 2:
        printf("%02x: %02x%02x\n", pc_, opcode, Read(pc_+1));
        break;
    case 3:
        printf("%02x: %02x%02x%02x\n", pc_, opcode, Read(pc_+1), Read(pc_+2));
        break;
    }
#endif

    //printf("Decoding instruction at %04x -> %02x\n", pc_, opcode);
    // Based on the AddressingMode of the instruction, compute the address
    // target to be used by the instruction.
    switch(AddressingMode(info.mode)) {
    case Absolute:
        addr = Read16(pc_+1);
        break;
    case AbsoluteX:
        addr = Read16(pc_+1) + x_;
        if (PagesDiffer(addr - x_, addr))
            cycles_ += info.page;
        break;
    case AbsoluteY:
        addr = Read16(pc_+1) + y_;
        if (PagesDiffer(addr - y_, addr))
            cycles_ += info.page;
        break;
    case IndexedIndirect:
        //addr = Read16Bug(Read(pc_+1) + x_);
        addr = Read16((Read(pc_ + 1) + x()) & 0xff);
        break;
    case Indirect:
        addr = Read16Bug(Read16(pc_+1));
        break;
    case IndirectIndexed:
        //addr = Read16Bug(Read(pc_+1)) + y_;
        // Fixed?
        addr = Read16(Read(pc_ + 1)) + y();
        if (PagesDiffer(addr - y_, addr))
            cycles_ += info.page;
        break;
    case ZeroPage:
        addr = Read(pc_ + 1);
        break;
    case ZeroPageX:
        addr = (Read(pc_ + 1) + x_) & 0xFF;
        break;
    case ZeroPageY:
        addr = (Read(pc_ + 1) + y_) & 0xFF;
        break;
    case Immediate:
        addr = pc_ + 1;
        break;
    case Accumulator:
    case Implied:
        addr = 0;
        break;
    case Relative:
        addr = pc_ + 2 + int8_t(Read(pc_ + 1));;
        break;
    }

#ifdef TESTCPU
    printf("Computed address %04x via mode %d (%02x %02x)\n",
            addr, info.mode, Read(addr), Read(addr+1));
#endif
    pc_ += info.size;
    cycles_ += info.cycles;

    switch(opcode) {
    /* BRK */
    case 0x0:
        Push16(pc_+1);
        Push(flags_.value | 0x10);
        flags_.i = 1;
        pc_ = Read16(0xFFFE);
        break;
    /* ORA (nn,X) */
    case 0x1:
    /* ORA nn */
    case 0x5:
    /* ORA #nn */
    case 0x9:
    /* ORA nnnn */
    case 0xD:
    /* ORA (nn,Y) */
    case 0x11:
    /* ORA nn,X */
    case 0x15:
    /* ORA nnnn,Y */
    case 0x19:
    /* ORA nnnn,X */
    case 0x1D:
        a_ = a_ | Read(addr);
        SetZN(a_);
        break;
    /* ASL nn */
    case 0x6:
    /* ASL nnnn */
    case 0xE:
    /* ASL nn,X */
    case 0x16:
    /* ASL nnnn,X */
    case 0x1E:
        val = Read(addr);
        flags_.c = val >> 7;
        val <<= 1;
        Write(addr, val);
        SetZN(val);
        break;
    /* ASL A */
    case 0xA:
        flags_.c = a_ >> 7;
        a_ <<= 1;
        SetZN(a_);
        break;
    /* PHP */
    case 0x8:
        Push(flags_.value | 0x10);
        break;
    /* BPL nn */
    case 0x10:
        if (!flags_.n)
            Branch(addr);
        break;
    /* CLC */
    case 0x18:
        flags_.c = 0;
        break;
    /* JSR */
    case 0x20:
        Push16(pc_ - 1);
        pc_ = addr;
        break;
    /* AND (nn,X) */
    case 0x21:
    /* AND nn */
    case 0x25:
    /* AND #nn */
    case 0x29:
    /* AND nnnn */
    case 0x2D:
    /* AND (nn,Y) */
    case 0x31:
    /* AND nn,X */
    case 0x35:
    /* AND nnnn,Y */
    case 0x39:
    /* AND nnnn,X */
    case 0x3D:
        a_ = a_ & Read(addr);
        SetZN(a_);
        break;
    /* BIT nn */
    case 0x24:
    /* BIT nnnn */
    case 0x2C:
        val = Read(addr);
        flags_.v = val >> 6;
        SetZ(val & a_);
        SetN(val);
        break;
    /* ROL nn */
    case 0x26:
    /* ROL nnnn */
    case 0x2E:
    /* ROL nn,X */
    case 0x36:
    /* ROL nnnn,X */
    case 0x3E:
        r = Read(addr);
        r = (r << 1) | flags_.c;
        flags_.c = r >> 8;
        Write(addr, r);
        SetZN(r);
        break;
    /* PLP */
    case 0x28:
        flags_.value = (Pull() & 0xEF) | 0x20;
        break;
    /* ROL A */
    case 0x2A:
        a = (a_ << 1) | flags_.c;
        flags_.c = a_ >> 7;
        a_ = a;
        SetZN(a_);
        break;
    /* BMI nn */
    case 0x30:
        if (flags_.n)
            Branch(addr);
        break;
    /* SEC */
    case 0x38:
        flags_.c = 1;
        break;
    /* RTI */
    case 0x40:
        flags_.value = (Pull() & 0xEF) | 0x20;
        pc_ = Pull16();
        break;
    /* EOR (nn,X) */
    case 0x41:
    /* EOR nn */
    case 0x45:
    /* EOR #nn */
    case 0x49:
    /* EOR nnnn */
    case 0x4D:
    /* EOR (nn,Y) */
    case 0x51:
    /* EOR nn,X */
    case 0x55:
    /* EOR nnnn,Y */
    case 0x59:
    /* EOR nnnn,X */
    case 0x5D:
        a_ = a_ ^ Read(addr);
        SetZN(a_);
        break;
    /* LSR nn */
    case 0x46:
    /* LSR nnnn */
    case 0x4E:
    /* LSR nn,X */
    case 0x56:
    /* LSR nnnn,X */
    case 0x5E:
        val = Read(addr);
        flags_.c = val & 1;
        val >>= 1;
        Write(addr, val);
        SetZN(val);
        break;
    /* PHA */
    case 0x48:
        Push(a_);
        break;
    /* BVC */
    case 0x50:
        if (!flags_.v)
            Branch(addr);
        break;
    /* JMP nnnn */
    case 0x4C:
    /* JMP (nnnn) */
    case 0x6C:
        pc_ = addr;
        break;
    /* LSR A */
    case 0x4A:
        flags_.c = a_ & 1;
        a_ >>= 1;
        SetZN(a_);
        break;
    /* CLI */
    case 0x58:
        flags_.i = 0;
        break;
    /* RTS */
    case 0x60:
        pc_ = Pull16() + 1;
        break;
    /* ADC (nn,X) */
    case 0x61:
    /* ADC nn */
    case 0x65:
    /* ADC #nn */
    case 0x69:
    /* ADC nnnn */
    case 0x6D:
    /* ADC (nn,Y) */
    case 0x71:
    /* ADC nn,X */
    case 0x75:
    /* ADC nnnn,Y */
    case 0x79:
    /* ADC nnnn,X */
    case 0x7D:
        a = a_;
        b = Read(addr);
        r = a + b + flags_.c;
        a_ = r;
        flags_.c = (r > 0xff);
        flags_.v = ((a ^ b) & 0x80) == 0 && ((a ^ a_) & 0x80) != 0;
        SetZN(a_);
        break;
    /* ROR nn */
    case 0x66:
    /* ROR nnnn */
    case 0x6E:
    /* ROR nn,X */
    case 0x76:
    /* ROR nnnn,X */
    case 0x7E:
        val = Read(addr);
        a = (val >> 1) | (flags_.c << 7);
        flags_.c = val & 1;
        Write(addr, a);
        SetZN(a);
        break;
    /* PLA */
    case 0x68:
        a_ = Pull();
        SetZN(a_);
        break;
    /* ROR A */
    case 0x6A:
        a = (a_ >> 1) | (flags_.c << 7);
        flags_.c = a_ & 1;
        a_ = a;
        SetZN(a_);
        break;
    /* BVS */
    case 0x70:
        if (flags_.v)
            Branch(addr);
        break;
    /* SEI */
    case 0x78:
        flags_.i = 1;
        break;
    /* STA (nn,X) */
    case 0x81:
    /* STA nn */
    case 0x85:
    /* STA nnnn */
    case 0x8D:
    /* STA (nn,Y) */
    case 0x91:
    /* STA nn,X */
    case 0x95:
    /* STA nnnn,Y */
    case 0x99:
    /* STA nnnn,X */
    case 0x9D:
        Write(addr, a_);
        break;
    /* STY nn */
    case 0x84:
    /* STY nnnn */
    case 0x8C:
    /* STY nn,X */
    case 0x94:
        Write(addr, y_);
        break;
    /* STX nn */
    case 0x86:
    /* STX nnnn */
    case 0x8E:
    /* STX nn,Y */
    case 0x96:
        Write(addr, x_);
        break;
    /* DEY */
    case 0x88:
        SetZN(--y_);
        break;
    /* TXA */
    case 0x8A:
        a_ = x_;
        SetZN(a_);
        break;
    /* BCC nn */
    case 0x90:
        if (!flags_.c)
            Branch(addr);
        break;
    /* TYA */
    case 0x98:
        a_ = y_;
        SetZN(a_);
        break;
    /* TXS */
    case 0x9A:
        sp_ = x_;
        break;
    /* LDY #nn */
    case 0xA0:
    /* LDY nn */
    case 0xA4:
    /* LDY nnnn */
    case 0xAC:
    /* LDY nn,X */
    case 0xB4:
    /* LDY nnnn,X */
    case 0xBC:
        y_ = Read(addr);
        SetZN(y_);
        break;
    /* LDA (nn,X) */
    case 0xA1:
    /* LDA nn */
    case 0xA5:
    /* LDA #nn */
    case 0xA9:
    /* LDA nnnn */
    case 0xAD:
    /* LDA (nn,Y) */
    case 0xB1:
    /* LDA nn,X */
    case 0xB5:
    /* LDA nnnn,Y */
    case 0xB9:
    /* LDA nnnn,X */
    case 0xBD:
        a_ = Read(addr);
        SetZN(a_);
        break;
    /* LDX #nn */
    case 0xA2:
    /* LDX nn */
    case 0xA6:
    /* LDX nnnn */
    case 0xAE:
    /* LDX nn,Y */
    case 0xB6:
    /* LDX nnnn,Y */
    case 0xBE:
        x_ = Read(addr);
        SetZN(x_);
        break;
    /* TAY */
    case 0xA8:
        y_ = a_;
        SetZN(y_);
        break;
    /* TAX */
    case 0xAA:
        x_ = a_;
        SetZN(x_);
        break;
    /* BCS nn */
    case 0xB0:
        if (flags_.c)
            Branch(addr);
        break;
    /* CLV */
    case 0xB8:
        flags_.v = 0;
        break;
    /* TSX */
    case 0xBA:
        x_ = sp_;
        SetZN(x_);
        break;
    /* CPY #nn */
    case 0xC0:
    /* CPY nn */
    case 0xC4:
    /* CPY nnnn */
    case 0xCC:
        Compare(y_, Read(addr));
        break;
    /* CMP (nn,X) */
    case 0xC1:
    /* CMP nn */
    case 0xC5:
    /* CMP #nn */
    case 0xC9:
    /* CMP nnnn */
    case 0xCD:
    /* CMP (nn,Y) */
    case 0xD1:
    /* CMP nn,X */
    case 0xD5:
    /* CMP nnnn,Y */
    case 0xD9:
    /* CMP nnnn,X */
    case 0xDD:
        Compare(a_, Read(addr));
        break;
    /* DEC nn */
    case 0xC6:
    /* DEC nnnn */
    case 0xCE:
    /* DEC nn,X */
    case 0xD6:
    /* DEC nnnn,X */
    case 0xDE:
        val = Read(addr) - 1;
        Write(addr, val);
        SetZN(val);
        break;
    /* INY */
    case 0xC8:
        SetZN(++y_);
        break;
    /* DEX */
    case 0xCA:
        SetZN(--x_);
        break;
    /* BNE nn */
    case 0xD0:
        if (!flags_.z)
            Branch(addr);
        break;
    /* CLD */
    case 0xD8:
        flags_.d = 0;
        break;
    /* CPX #nn */
    case 0xE0:
    /* CPX nn */
    case 0xE4:
    /* CPX nnnn */
    case 0xEC:
        Compare(x_, Read(addr));
        break;
    /* SBC (nn,X) */
    case 0xE1:
    /* SBC nn */
    case 0xE5:
    /* SBC #nn */
    case 0xE9:
    /* SBC nnnn */
    case 0xED:
    /* SBC (nn,Y) */
    case 0xF1:
    /* SBC nn,X */
    case 0xF5:
    /* SBC nnnn,Y */
    case 0xF9:
    /* SBC nnnn,X */
    case 0xFD:
        a = a_;
        b = Read(addr);
        r = a - b - (1- flags_.c);
        a_ = r;
        flags_.c = (r >= 0);
        flags_.v = ((a ^ r) & 0x80) != 0 && ((a ^ b) & 0x80) != 0;
        SetZN(a_);
        break;
    /* INC nn */
    case 0xE6:
    /* INC nnnn */
    case 0xEE:
    /* INC nn,X */
    case 0xF6:
    /* INC nnnn,X */
    case 0xFE:
        val = Read(addr) + 1;
        Write(addr, val);
        SetZN(val);
        break;
    /* INX */
    case 0xE8:
        SetZN(++x_);
        break;
    /* NOP */
    case 0xEA:
        break;
    /* BEQ nn */
    case 0xF0:
        if (flags_.z)
            Branch(addr);
        break;
    /* SED */
    case 0xF8:
        flags_.d = true;
        break;
    /* Unknown or illegal instruction */
    default:
        fprintf(stderr, "Illegal opcode %02x at %04x\n", opcode, fetchpc);
        halted_ = true;
        Flush();
    }
#if 0
    if (profile_) {
        (*profile_->mutable_count())[pc_] += (cycles_ - cycles);
    }
#endif
    return cycles_ - cycles;
}

// Information about each instruction is encoded into the info_ table.
// Each 4 bits means (from lowest to highest):
//    AddressingMode
//    Instruction Size (in bytes)
//    Cyles
//    Extra cycles when crossing a page boundary
const Cpu::InstructionInfo Cpu::info_[256] = {
    // 0x00      1       2       3       4       5       6       7
    //    8      9       a       b       c       d       e       f
    0x0715, 0x0626, 0x0205, 0x0806, 0x032a, 0x032a, 0x052a, 0x050a, 
    0x0315, 0x0224, 0x0213, 0x0204, 0x0430, 0x0430, 0x0630, 0x0600, 
    // 0x10
    0x1229, 0x1528, 0x0205, 0x0808, 0x042b, 0x042b, 0x062b, 0x060b, 
    0x0215, 0x1432, 0x0215, 0x0702, 0x1431, 0x1431, 0x0731, 0x0701, 
    // 0x20
    0x0630, 0x0626, 0x0205, 0x0806, 0x032a, 0x032a, 0x052a, 0x050a, 
    0x0415, 0x0224, 0x0213, 0x0204, 0x0430, 0x0430, 0x0630, 0x0600, 
    // 0x30
    0x1229, 0x1528, 0x0205, 0x0808, 0x042b, 0x042b, 0x062b, 0x060b, 
    0x0215, 0x1432, 0x0215, 0x0702, 0x1431, 0x1431, 0x0731, 0x0701, 
    // 0x40
    0x0615, 0x0626, 0x0205, 0x0806, 0x032a, 0x032a, 0x052a, 0x050a, 
    0x0315, 0x0224, 0x0213, 0x0204, 0x0330, 0x0430, 0x0630, 0x0600, 
    // 0x50
    0x1229, 0x1528, 0x0205, 0x0808, 0x042b, 0x042b, 0x062b, 0x060b, 
    0x0215, 0x1432, 0x0215, 0x0702, 0x1431, 0x1431, 0x0731, 0x0701, 
    // 0x60
    0x0615, 0x0626, 0x0205, 0x0806, 0x032a, 0x032a, 0x052a, 0x050a, 
    0x0415, 0x0224, 0x0213, 0x0204, 0x0537, 0x0430, 0x0630, 0x0600, 
    // 0x70
    0x1229, 0x1528, 0x0205, 0x0808, 0x042b, 0x042b, 0x062b, 0x060b, 
    0x0215, 0x1432, 0x0215, 0x0702, 0x1431, 0x1431, 0x0731, 0x0701, 
    // 0x80
    0x0224, 0x0626, 0x0204, 0x0606, 0x032a, 0x032a, 0x032a, 0x030a, 
    0x0215, 0x0204, 0x0215, 0x0204, 0x0430, 0x0430, 0x0430, 0x0400, 
    // 0x90
    0x1229, 0x0628, 0x0205, 0x0608, 0x042b, 0x042b, 0x042c, 0x040c, 
    0x0215, 0x0532, 0x0215, 0x0502, 0x0501, 0x0531, 0x0502, 0x0502, 
    // 0xA0
    0x0224, 0x0626, 0x0224, 0x0606, 0x032a, 0x032a, 0x032a, 0x030a, 
    0x0215, 0x0224, 0x0215, 0x0204, 0x0430, 0x0430, 0x0430, 0x0400, 
    // 0xB0
    0x1229, 0x1528, 0x0205, 0x1508, 0x042b, 0x042b, 0x042c, 0x040c, 
    0x0215, 0x1432, 0x0215, 0x1402, 0x1431, 0x1431, 0x1432, 0x1402, 
    // 0xC0
    0x0224, 0x0626, 0x0204, 0x0806, 0x032a, 0x032a, 0x052a, 0x050a, 
    0x0215, 0x0224, 0x0215, 0x0204, 0x0430, 0x0430, 0x0630, 0x0600, 
    // 0xD0
    0x1229, 0x1528, 0x0205, 0x0808, 0x042b, 0x042b, 0x062b, 0x060b, 
    0x0215, 0x1432, 0x0215, 0x0702, 0x1431, 0x1431, 0x0731, 0x0701, 
    // 0xE0
    0x0224, 0x0626, 0x0204, 0x0806, 0x032a, 0x032a, 0x052a, 0x050a, 
    0x0215, 0x0224, 0x0215, 0x0204, 0x0430, 0x0430, 0x0630, 0x0600, 
    // 0xF0
    0x1229, 0x1528, 0x0205, 0x0808, 0x042b, 0x042b, 0x062b, 0x060b, 
    0x0215, 0x1432, 0x0215, 0x0702, 0x1431, 0x1431, 0x0731, 0x0701, 
};

const char* Cpu::instruction_names_[] = {
/* 00 */      "BRK",
/* 01 */      "ORA ($%02x,X)",
/* 02 */      "illop_02",
/* 03 */      "illop_03",
/* 04 */      "illop_04",
/* 05 */      "ORA $%02x",
/* 06 */      "ASL $%02x",
/* 07 */      "illop_07",
/* 08 */      "PHP",
/* 09 */      "ORA #$%02x",
/* 0a */      "ASL A",
/* 0b */      "illop_0b",
/* 0c */      "illop_0c",
/* 0d */      "ORA $%04x",
/* 0e */      "ASL $%04x",
/* 0f */      "illop_0f",
/* 10 */      "BPL $%02x",
/* 11 */      "ORA ($%02x,Y)",
/* 12 */      "illop_12",
/* 13 */      "illop_13",
/* 14 */      "illop_14",
/* 15 */      "ORA $%02x,X",
/* 16 */      "ASL $%02x,X",
/* 17 */      "illop_17",
/* 18 */      "CLC",
/* 19 */      "ORA $%04x,Y",
/* 1a */      "illop_1a",
/* 1b */      "illop_1b",
/* 1c */      "illop_1c",
/* 1d */      "ORA $%04x,X",
/* 1e */      "ASL $%04x,X",
/* 1f */      "illop_1f",
/* 20 */      "JSR $%04x",
/* 21 */      "AND ($%02x,X)",
/* 22 */      "illop_22",
/* 23 */      "illop_23",
/* 24 */      "BIT $%02x",
/* 25 */      "AND $%02x",
/* 26 */      "ROL $%02x",
/* 27 */      "illop_27",
/* 28 */      "PLP",
/* 29 */      "AND #$%02x",
/* 2a */      "ROL A",
/* 2b */      "illop_2b",
/* 2c */      "BIT $%04x",
/* 2d */      "AND $%04x",
/* 2e */      "ROL $%04x",
/* 2f */      "illop_2f",
/* 30 */      "BMI $%02x",
/* 31 */      "AND ($%02x,Y)",
/* 32 */      "illop_32",
/* 33 */      "illop_33",
/* 34 */      "illop_34",
/* 35 */      "AND $%02x,X",
/* 36 */      "ROL $%02x,X",
/* 37 */      "illop_37",
/* 38 */      "SEC",
/* 39 */      "AND $%04x,Y",
/* 3a */      "illop_3a",
/* 3b */      "illop_3b",
/* 3c */      "illop_3c",
/* 3d */      "AND $%04x,X",
/* 3e */      "ROL $%04x,X",
/* 3f */      "illop_3f",
/* 40 */      "RTI",
/* 41 */      "EOR ($%02x,X)",
/* 42 */      "illop_42",
/* 43 */      "illop_43",
/* 44 */      "illop_44",
/* 45 */      "EOR $%02x",
/* 46 */      "LSR $%02x",
/* 47 */      "illop_47",
/* 48 */      "PHA",
/* 49 */      "EOR #$%02x",
/* 4a */      "LSR A",
/* 4b */      "illop_4b",
/* 4c */      "JMP $%04x",
/* 4d */      "EOR $%04x",
/* 4e */      "LSR $%04x",
/* 4f */      "illop_4f",
/* 50 */      "BVC",
/* 51 */      "EOR ($%02x,Y)",
/* 52 */      "illop_52",
/* 53 */      "illop_53",
/* 54 */      "illop_54",
/* 55 */      "EOR $%02x,X",
/* 56 */      "LSR $%02x,X",
/* 57 */      "illop_57",
/* 58 */      "CLI",
/* 59 */      "EOR $%04x,Y",
/* 5a */      "illop_5a",
/* 5b */      "illop_5b",
/* 5c */      "illop_5c",
/* 5d */      "EOR $%04x,X",
/* 5e */      "LSR $%04x,X",
/* 5f */      "illop_5f",
/* 60 */      "RTS",
/* 61 */      "ADC ($%02x,X)",
/* 62 */      "illop_62",
/* 63 */      "illop_63",
/* 64 */      "illop_64",
/* 65 */      "ADC $%02x",
/* 66 */      "ROR $%02x",
/* 67 */      "illop_67",
/* 68 */      "PLA",
/* 69 */      "ADC #$%02x",
/* 6a */      "ROR A",
/* 6b */      "illop_6b",
/* 6c */      "JMP ($%04x)",
/* 6d */      "ADC $%04x",
/* 6e */      "ROR $%04x",
/* 6f */      "illop_6f",
/* 70 */      "BVS",
/* 71 */      "ADC ($%02x,Y)",
/* 72 */      "illop_72",
/* 73 */      "illop_73",
/* 74 */      "illop_74",
/* 75 */      "ADC $%02x,X",
/* 76 */      "ROR $%02x,X",
/* 77 */      "illop_77",
/* 78 */      "SEI",
/* 79 */      "ADC $%04x,Y",
/* 7a */      "illop_7a",
/* 7b */      "illop_7b",
/* 7c */      "illop_7c",
/* 7d */      "ADC $%04x,X",
/* 7e */      "ROR $%04x,X",
/* 7f */      "illop_7f",
/* 80 */      "illop_80",
/* 81 */      "STA ($%02x,X)",
/* 82 */      "illop_82",
/* 83 */      "illop_83",
/* 84 */      "STY $%02x",
/* 85 */      "STA $%02x",
/* 86 */      "STX $%02x",
/* 87 */      "illop_87",
/* 88 */      "DEY",
/* 89 */      "illop_89",
/* 8a */      "TXA",
/* 8b */      "illop_8b",
/* 8c */      "STY $%04x",
/* 8d */      "STA $%04x",
/* 8e */      "STX $%04x",
/* 8f */      "illop_8f",
/* 90 */      "BCC $%02x",
/* 91 */      "STA ($%02x,Y)",
/* 92 */      "illop_92",
/* 93 */      "illop_93",
/* 94 */      "STY $%02x,X",
/* 95 */      "STA $%02x,X",
/* 96 */      "STX $%02x,Y",
/* 97 */      "illop_97",
/* 98 */      "TYA",
/* 99 */      "STA $%04x,Y",
/* 9a */      "TXS",
/* 9b */      "illop_9b",
/* 9c */      "illop_9c",
/* 9d */      "STA $%04x,X",
/* 9e */      "illop_9e",
/* 9f */      "illop_9f",
/* a0 */      "LDY #$%02x",
/* a1 */      "LDA ($%02x,X)",
/* a2 */      "LDX #$%02x",
/* a3 */      "illop_a3",
/* a4 */      "LDY $%02x",
/* a5 */      "LDA $%02x",
/* a6 */      "LDX $%02x",
/* a7 */      "illop_a7",
/* a8 */      "TAY",
/* a9 */      "LDA #$%02x",
/* aa */      "TAX",
/* ab */      "illop_ab",
/* ac */      "LDY $%04x",
/* ad */      "LDA $%04x",
/* ae */      "LDX $%04x",
/* af */      "illop_af",
/* b0 */      "BCS $%02x",
/* b1 */      "LDA ($%02x,Y)",
/* b2 */      "illop_b2",
/* b3 */      "illop_b3",
/* b4 */      "LDY $%02x,X",
/* b5 */      "LDA $%02x,X",
/* b6 */      "LDX $%02x,Y",
/* b7 */      "illop_b7",
/* b8 */      "CLV",
/* b9 */      "LDA $%04x,Y",
/* ba */      "TSX",
/* bb */      "illop_bb",
/* bc */      "LDY $%04x,X",
/* bd */      "LDA $%04x,X",
/* be */      "LDX $%04x,Y",
/* bf */      "illop_bf",
/* c0 */      "CPY #$%02x",
/* c1 */      "CMP ($%02x,X)",
/* c2 */      "illop_c2",
/* c3 */      "illop_c3",
/* c4 */      "CPY $%02x",
/* c5 */      "CMP $%02x",
/* c6 */      "DEC $%02x",
/* c7 */      "illop_c7",
/* c8 */      "INY",
/* c9 */      "CMP #$%02x",
/* ca */      "DEX",
/* cb */      "illop_cb",
/* cc */      "CPY $%04x",
/* cd */      "CMP $%04x",
/* ce */      "DEC $%04x",
/* cf */      "illop_cf",
/* d0 */      "BNE $%02x",
/* d1 */      "CMP ($%02x,Y)",
/* d2 */      "illop_d2",
/* d3 */      "illop_d3",
/* d4 */      "illop_d4",
/* d5 */      "CMP $%02x,X",
/* d6 */      "DEC $%02x,X",
/* d7 */      "illop_d7",
/* d8 */      "CLD",
/* d9 */      "CMP $%04x,Y",
/* da */      "illop_da",
/* db */      "illop_db",
/* dc */      "illop_dc",
/* dd */      "CMP $%04x,X",
/* de */      "DEC $%04x,X",
/* df */      "illop_df",
/* e0 */      "CPX #$%02x",
/* e1 */      "SBC ($%02x,X)",
/* e2 */      "illop_e2",
/* e3 */      "illop_e3",
/* e4 */      "CPX $%02x",
/* e5 */      "SBC $%02x",
/* e6 */      "INC $%02x",
/* e7 */      "illop_e7",
/* e8 */      "INX",
/* e9 */      "SBC #$%02x",
/* ea */      "NOP",
/* eb */      "illop_eb",
/* ec */      "CPX $%04x",
/* ed */      "SBC $%04x",
/* ee */      "INC $%04x",
/* ef */      "illop_ef",
/* f0 */      "BEQ $%02x",
/* f1 */      "SBC ($%02x,Y)",
/* f2 */      "illop_f2",
/* f3 */      "illop_f3",
/* f4 */      "illop_f4",
/* f5 */      "SBC $%02x,X",
/* f6 */      "INC $%02x,X",
/* f7 */      "illop_f7",
/* f8 */      "SED",
/* f9 */      "SBC $%04x,Y",
/* fa */      "illop_fa",
/* fb */      "illop_fb",
/* fc */      "illop_fc",
/* fd */      "SBC $%04x,X",
/* fe */      "INC $%04x,X",
/* ff */      "illop_ff",
};
}  // namespace protones
