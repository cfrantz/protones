#ifndef PROTONES_CPU2_H
#define PROTONES_CPU2_H
#include <cstdint>
#include <functional>
#include <string>
#include "nes/base.h"
#include "nes/mem.h"
#include "proto/cpu6502.pb.h"
namespace protones {

class Cpu : public EmulatedDevice {
  public:
    typedef std::function<uint8_t(Cpu*, uint16_t, uint8_t)> MemoryCb;
    typedef std::function<uint16_t(Cpu*)> ExecCb;
    Cpu() : Cpu(nullptr) {}
    Cpu(Mem* mem);
    ~Cpu() { }

    void SaveState(proto::CPU6502 *state);
    void LoadState(proto::CPU6502 *state);
    void LoadEverdriveState(const uint8_t* state);
    void Reset();
    void Emulate() { Execute(); }
    int Execute();
    void Stall(int s) { stall_ += s; }
    std::string Disassemble(uint16_t *nexti=nullptr, bool tracemode=false);
    std::string CpuState();
    inline void NMI() {
        nmi_pending_ = true;
        Emit("NMI");
    }
    inline void IRQ() {
        irq_pending_ = true;
        Emit("IRQ");
    }
    void Flush();
    inline bool irq_pending() const { return irq_pending_; }

    inline void reset() { Reset(); }
    inline void nmi() { NMI(); }
    inline void irq() { IRQ(); }
    inline void memory(Mem* mem) { mem_ = mem; }

    inline uint64_t cycles() { return cycles_; }

    inline uint8_t a() { return a_; }
    inline uint8_t x() { return x_; }
    inline uint8_t y() { return y_; }
    inline uint8_t sp() { return sp_; }
    inline uint8_t flags() { return flags_.value; }
    inline uint16_t pc() { return pc_; }

    inline void set_a(uint8_t v) { a_ = v; }
    inline void set_x(uint8_t v) { x_ = v; }
    inline void set_y(uint8_t v) { y_ = v; }
    inline void set_sp(uint8_t v) { sp_ = v; }
    inline void set_flags(uint8_t v) { flags_.value = v; }
    inline void set_pc(uint16_t v) { pc_ = v; }

    inline bool cf() { return flags_.c; }
    inline bool zf() { return flags_.z; }
    inline bool idf() { return flags_.i; }
    inline bool dmf() { return flags_.d; }
    inline bool bcf() { return flags_.b; }
    inline bool of() { return flags_.v; }
    inline bool nf() { return flags_.n; }

    union CpuFlags {
        uint8_t value;
        struct {
            uint8_t c:1;
            uint8_t z:1;
            uint8_t i:1;
            uint8_t d:1;
            uint8_t b:1;
            uint8_t u:1;
            uint8_t v:1;
            uint8_t n:1;
        };
    };

    union InstructionInfo {
        uint16_t value;
        struct {
            uint16_t mode:4;
            uint16_t size:4;
            uint16_t cycles:4;
            uint16_t page:4;
        };
    };

    enum AddressingMode {
        Absolute,
        AbsoluteX,
        AbsoluteY,
        Accumulator,
        Immediate,
        Implied,
        IndexedIndirect,
        Indirect,
        IndirectIndexed,
        Relative,
        ZeroPage,
        ZeroPageX,
        ZeroPageY,
    };

    enum RWLog {
        LogRead = 1,
        LogWrite = 2,
        LogWriteZero = 4,
        LogStack = 8,
        LogExec = 16,
    };

    inline void set_read_cb(uint16_t addr, const MemoryCb& cb) {
        read_cb_[addr] = cb;
    }
    inline void set_write_cb(uint16_t addr, const MemoryCb& cb) {
        write_cb_[addr] = cb;
    }
    inline void set_exec_cb(uint16_t addr, const ExecCb& cb) {
        exec_cb_[addr] = cb;
    }
    void SaveRwLog();
    void ClearRwLog() {
        memset(rwlog_, 0, sizeof(rwlog_));
    }

  private:
    uint8_t inline Read(uint16_t addr, RWLog how=LogExec) {
        uint8_t val = mem_->read_byte(addr);
        if (read_cb_[addr]) val = read_cb_[addr](this, addr, val);
        rwlog_[addr] |= how;
        return val;
    }
    void inline Write(uint16_t addr, uint8_t val, RWLog how=LogWrite) {
        if (write_cb_[addr]) val = write_cb_[addr](this, addr, val);
        mem_->write_byte(addr, val);
        if (how == LogWrite && val == 0) {
            how = LogWriteZero;
        }
        rwlog_[addr] |= how;
    }
    uint16_t inline Read16(uint16_t addr) {
        return Read(addr) | Read(addr+1) << 8;
    }
    uint16_t inline Read16Bug(uint16_t addr) {
        // When reading the high byte of the word, the address
        // increments, but doesn't carry from the low address byte to the
        // high address byte.
        uint16_t ret = Read(addr);
        ret |= Read((addr & 0xFF00) | ((addr+1) & 0x00FF)) << 8;
        return ret;
    }

    inline void Push(uint8_t val) { Write(sp_-- | 0x100, val, LogStack); }
    inline uint8_t Pull() { return Read(++sp_ | 0x100, LogStack); }

    inline void Push16(uint16_t val) { Push(val>>8); Push(val); }
    inline uint16_t Pull16() { return Pull() | Pull() << 8; }

    inline void SetZ(uint8_t val) { flags_.z = (val == 0); }
    inline void SetN(uint8_t val) { flags_.n = !!(val & 0x80); }
    inline void SetZN(uint8_t val) { SetZ(val); SetN(val); }
    inline void Compare(uint8_t a, uint8_t b) {
        SetZN(a - b);
        flags_.c = (a >= b);
    }
    inline bool PagesDiffer(uint16_t a, uint16_t b) {
        return (a & 0xFF00) != (b & 0xFF00);
    }
    void Branch(uint16_t addr);


    Mem* mem_;
    CpuFlags flags_;
    uint16_t pc_;
    uint8_t sp_;
    uint8_t a_, x_, y_;
    
    uint64_t cycles_;
    int stall_;
    bool nmi_pending_;
    bool irq_pending_;

    static const InstructionInfo info_[256];
    static const char* instruction_names_[256];

    void Emit(const char *buf, int how=0);
    void Trace();

    static const int TRACEBUFSZ = 1000000;
    static const int SLOP = 1000;
    char tracebuf_[TRACEBUFSZ][80];
    int tbptr_;
    bool halted_;
    std::map<uint16_t, MemoryCb> read_cb_;
    std::map<uint16_t, MemoryCb> write_cb_;
    std::map<uint16_t, ExecCb> exec_cb_;

    uint8_t rwlog_[65536];
};

}  // namespace protones
#endif // PROTONES_CPU2_H
