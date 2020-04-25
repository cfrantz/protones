use protones::nes::cpu6502::Cpu6502;
use protones::nes::cpu6502::Memory;

struct Ram {
    mem: Vec<u8>,
}

impl Memory for Ram {
    fn read(&mut self, address: u16) -> u8 {
        self.mem[address as usize]
    }
    fn write(&mut self, address: u16, value: u8) {
        self.mem[address as usize] = value;
    }
}

#[test]
fn self_test_program() {
    let prg = include_bytes!("6502_functional_test.bin");
    let mut mem = Vec::new();
    mem.resize(65536, 0);

    for (i, val) in prg.iter().enumerate() {
        mem[i + 0x400] = *val;
    }

    mem[0xFFFC] = 0x00;
    mem[0xFFFD] = 0x04;

    let mut ram = Ram { mem: mem };
    let mut cpu = Cpu6502::default();
    cpu.reset();

    let mut last_pc = 0xFFFFu16;

    while cpu.get_pc() != last_pc {
        last_pc = cpu.get_pc();
        //println!("{}", cpu.cpustate());
        //println!("{}", Cpu6502::disassemble(&mut ram, last_pc).0);
        cpu.execute(&mut ram);
    }
    println!("Test executed {} 6502 cycles.", cpu.get_cycles());
    assert_eq!(last_pc, 0x3691);
}
