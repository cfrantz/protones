#[derive(Clone, Copy, Debug)]
pub enum AddressingMode {
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
}

#[derive(Clone, Copy, Debug)]
pub struct InstructionInfo {
    pub size: u8,
    pub cycles: u8,
    pub page: u8,
    pub mode: AddressingMode,
}

pub const INFO: [InstructionInfo; 256] = [
    InstructionInfo {
        size: 1,
        cycles: 7,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 2,
        cycles: 6,
        page: 0,
        mode: AddressingMode::IndexedIndirect,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 8,
        page: 0,
        mode: AddressingMode::IndexedIndirect,
    },
    InstructionInfo {
        size: 2,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 2,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 2,
        cycles: 5,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 0,
        cycles: 5,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 1,
        cycles: 3,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Accumulator,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 3,
        cycles: 6,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 0,
        cycles: 6,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 1,
        mode: AddressingMode::Relative,
    },
    InstructionInfo {
        size: 2,
        cycles: 5,
        page: 1,
        mode: AddressingMode::IndirectIndexed,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 8,
        page: 0,
        mode: AddressingMode::IndirectIndexed,
    },
    InstructionInfo {
        size: 2,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 2,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 2,
        cycles: 6,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 0,
        cycles: 6,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 7,
        page: 0,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 3,
        cycles: 7,
        page: 0,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 0,
        cycles: 7,
        page: 0,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 3,
        cycles: 6,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 2,
        cycles: 6,
        page: 0,
        mode: AddressingMode::IndexedIndirect,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 8,
        page: 0,
        mode: AddressingMode::IndexedIndirect,
    },
    InstructionInfo {
        size: 2,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 2,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 2,
        cycles: 5,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 0,
        cycles: 5,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 1,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Accumulator,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 3,
        cycles: 6,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 0,
        cycles: 6,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 1,
        mode: AddressingMode::Relative,
    },
    InstructionInfo {
        size: 2,
        cycles: 5,
        page: 1,
        mode: AddressingMode::IndirectIndexed,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 8,
        page: 0,
        mode: AddressingMode::IndirectIndexed,
    },
    InstructionInfo {
        size: 2,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 2,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 2,
        cycles: 6,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 0,
        cycles: 6,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 7,
        page: 0,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 3,
        cycles: 7,
        page: 0,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 0,
        cycles: 7,
        page: 0,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 1,
        cycles: 6,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 2,
        cycles: 6,
        page: 0,
        mode: AddressingMode::IndexedIndirect,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 8,
        page: 0,
        mode: AddressingMode::IndexedIndirect,
    },
    InstructionInfo {
        size: 2,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 2,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 2,
        cycles: 5,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 0,
        cycles: 5,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 1,
        cycles: 3,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Accumulator,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 3,
        cycles: 3,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 3,
        cycles: 6,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 0,
        cycles: 6,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 1,
        mode: AddressingMode::Relative,
    },
    InstructionInfo {
        size: 2,
        cycles: 5,
        page: 1,
        mode: AddressingMode::IndirectIndexed,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 8,
        page: 0,
        mode: AddressingMode::IndirectIndexed,
    },
    InstructionInfo {
        size: 2,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 2,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 2,
        cycles: 6,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 0,
        cycles: 6,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 7,
        page: 0,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 3,
        cycles: 7,
        page: 0,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 0,
        cycles: 7,
        page: 0,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 1,
        cycles: 6,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 2,
        cycles: 6,
        page: 0,
        mode: AddressingMode::IndexedIndirect,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 8,
        page: 0,
        mode: AddressingMode::IndexedIndirect,
    },
    InstructionInfo {
        size: 2,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 2,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 2,
        cycles: 5,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 0,
        cycles: 5,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 1,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Accumulator,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 3,
        cycles: 5,
        page: 0,
        mode: AddressingMode::Indirect,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 3,
        cycles: 6,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 0,
        cycles: 6,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 1,
        mode: AddressingMode::Relative,
    },
    InstructionInfo {
        size: 2,
        cycles: 5,
        page: 1,
        mode: AddressingMode::IndirectIndexed,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 8,
        page: 0,
        mode: AddressingMode::IndirectIndexed,
    },
    InstructionInfo {
        size: 2,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 2,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 2,
        cycles: 6,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 0,
        cycles: 6,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 7,
        page: 0,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 3,
        cycles: 7,
        page: 0,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 0,
        cycles: 7,
        page: 0,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 2,
        cycles: 6,
        page: 0,
        mode: AddressingMode::IndexedIndirect,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 0,
        cycles: 6,
        page: 0,
        mode: AddressingMode::IndexedIndirect,
    },
    InstructionInfo {
        size: 2,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 2,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 2,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 0,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 0,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 1,
        mode: AddressingMode::Relative,
    },
    InstructionInfo {
        size: 2,
        cycles: 6,
        page: 0,
        mode: AddressingMode::IndirectIndexed,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 6,
        page: 0,
        mode: AddressingMode::IndirectIndexed,
    },
    InstructionInfo {
        size: 2,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 2,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 2,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageY,
    },
    InstructionInfo {
        size: 0,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageY,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 3,
        cycles: 5,
        page: 0,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 5,
        page: 0,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 0,
        cycles: 5,
        page: 0,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 3,
        cycles: 5,
        page: 0,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 0,
        cycles: 5,
        page: 0,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 0,
        cycles: 5,
        page: 0,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 2,
        cycles: 6,
        page: 0,
        mode: AddressingMode::IndexedIndirect,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 0,
        cycles: 6,
        page: 0,
        mode: AddressingMode::IndexedIndirect,
    },
    InstructionInfo {
        size: 2,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 2,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 2,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 0,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 0,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 1,
        mode: AddressingMode::Relative,
    },
    InstructionInfo {
        size: 2,
        cycles: 5,
        page: 1,
        mode: AddressingMode::IndirectIndexed,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 5,
        page: 1,
        mode: AddressingMode::IndirectIndexed,
    },
    InstructionInfo {
        size: 2,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 2,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 2,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageY,
    },
    InstructionInfo {
        size: 0,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageY,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 0,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 2,
        cycles: 6,
        page: 0,
        mode: AddressingMode::IndexedIndirect,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 0,
        cycles: 8,
        page: 0,
        mode: AddressingMode::IndexedIndirect,
    },
    InstructionInfo {
        size: 2,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 2,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 2,
        cycles: 5,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 0,
        cycles: 5,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 3,
        cycles: 6,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 0,
        cycles: 6,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 1,
        mode: AddressingMode::Relative,
    },
    InstructionInfo {
        size: 2,
        cycles: 5,
        page: 1,
        mode: AddressingMode::IndirectIndexed,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 8,
        page: 0,
        mode: AddressingMode::IndirectIndexed,
    },
    InstructionInfo {
        size: 2,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 2,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 2,
        cycles: 6,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 0,
        cycles: 6,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 7,
        page: 0,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 3,
        cycles: 7,
        page: 0,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 0,
        cycles: 7,
        page: 0,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 2,
        cycles: 6,
        page: 0,
        mode: AddressingMode::IndexedIndirect,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 0,
        cycles: 8,
        page: 0,
        mode: AddressingMode::IndexedIndirect,
    },
    InstructionInfo {
        size: 2,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 2,
        cycles: 3,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 2,
        cycles: 5,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 0,
        cycles: 5,
        page: 0,
        mode: AddressingMode::ZeroPage,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Immediate,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 3,
        cycles: 6,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 0,
        cycles: 6,
        page: 0,
        mode: AddressingMode::Absolute,
    },
    InstructionInfo {
        size: 2,
        cycles: 2,
        page: 1,
        mode: AddressingMode::Relative,
    },
    InstructionInfo {
        size: 2,
        cycles: 5,
        page: 1,
        mode: AddressingMode::IndirectIndexed,
    },
    InstructionInfo {
        size: 0,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 8,
        page: 0,
        mode: AddressingMode::IndirectIndexed,
    },
    InstructionInfo {
        size: 2,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 2,
        cycles: 4,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 2,
        cycles: 6,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 0,
        cycles: 6,
        page: 0,
        mode: AddressingMode::ZeroPageX,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 1,
        cycles: 2,
        page: 0,
        mode: AddressingMode::Implied,
    },
    InstructionInfo {
        size: 0,
        cycles: 7,
        page: 0,
        mode: AddressingMode::AbsoluteY,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 3,
        cycles: 4,
        page: 1,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 3,
        cycles: 7,
        page: 0,
        mode: AddressingMode::AbsoluteX,
    },
    InstructionInfo {
        size: 0,
        cycles: 7,
        page: 0,
        mode: AddressingMode::AbsoluteX,
    },
];

pub const NAMES: [&str; 256] = [
    "BRK",
    "ORA ($@,X)",
    "illop_02",
    "illop_03",
    "illop_04",
    "ORA $@",
    "ASL $@",
    "illop_07",
    "PHP",
    "ORA #$@",
    "ASL A",
    "illop_0b",
    "illop_0c",
    "ORA $@",
    "ASL $@",
    "illop_0f",
    "BPL $@",
    "ORA ($@,Y)",
    "illop_12",
    "illop_13",
    "illop_14",
    "ORA $@,X",
    "ASL $@,X",
    "illop_17",
    "CLC",
    "ORA $@,Y",
    "illop_1a",
    "illop_1b",
    "illop_1c",
    "ORA $@,X",
    "ASL $@,X",
    "illop_1f",
    "JSR $@",
    "AND ($@,X)",
    "illop_22",
    "illop_23",
    "BIT $@",
    "AND $@",
    "ROL $@",
    "illop_27",
    "PLP",
    "AND #$@",
    "ROL A",
    "illop_2b",
    "BIT $@",
    "AND $@",
    "ROL $@",
    "illop_2f",
    "BMI $@",
    "AND ($@,Y)",
    "illop_32",
    "illop_33",
    "illop_34",
    "AND $@,X",
    "ROL $@,X",
    "illop_37",
    "SEC",
    "AND $@,Y",
    "illop_3a",
    "illop_3b",
    "illop_3c",
    "AND $@,X",
    "ROL $@,X",
    "illop_3f",
    "RTI",
    "EOR ($@,X)",
    "illop_42",
    "illop_43",
    "illop_44",
    "EOR $@",
    "LSR $@",
    "illop_47",
    "PHA",
    "EOR #$@",
    "LSR A",
    "illop_4b",
    "JMP $@",
    "EOR $@",
    "LSR $@",
    "illop_4f",
    "BVC",
    "EOR ($@,Y)",
    "illop_52",
    "illop_53",
    "illop_54",
    "EOR $@,X",
    "LSR $@,X",
    "illop_57",
    "CLI",
    "EOR $@,Y",
    "illop_5a",
    "illop_5b",
    "illop_5c",
    "EOR $@,X",
    "LSR $@,X",
    "illop_5f",
    "RTS",
    "ADC ($@,X)",
    "illop_62",
    "illop_63",
    "illop_64",
    "ADC $@",
    "ROR $@",
    "illop_67",
    "PLA",
    "ADC #$@",
    "ROR A",
    "illop_6b",
    "JMP ($@)",
    "ADC $@",
    "ROR $@",
    "illop_6f",
    "BVS",
    "ADC ($@,Y)",
    "illop_72",
    "illop_73",
    "illop_74",
    "ADC $@,X",
    "ROR $@,X",
    "illop_77",
    "SEI",
    "ADC $@,Y",
    "illop_7a",
    "illop_7b",
    "illop_7c",
    "ADC $@,X",
    "ROR $@,X",
    "illop_7f",
    "illop_80",
    "STA ($@,X)",
    "illop_82",
    "illop_83",
    "STY $@",
    "STA $@",
    "STX $@",
    "illop_87",
    "DEY",
    "illop_89",
    "TXA",
    "illop_8b",
    "STY $@",
    "STA $@",
    "STX $@",
    "illop_8f",
    "BCC $@",
    "STA ($@,Y)",
    "illop_92",
    "illop_93",
    "STY $@,X",
    "STA $@,X",
    "STX $@,Y",
    "illop_97",
    "TYA",
    "STA $@,Y",
    "TXS",
    "illop_9b",
    "illop_9c",
    "STA $@,X",
    "illop_9e",
    "illop_9f",
    "LDY #$@",
    "LDA ($@,X)",
    "LDX #$@",
    "illop_a3",
    "LDY $@",
    "LDA $@",
    "LDX $@",
    "illop_a7",
    "TAY",
    "LDA #$@",
    "TAX",
    "illop_ab",
    "LDY $@",
    "LDA $@",
    "LDX $@",
    "illop_af",
    "BCS $@",
    "LDA ($@,Y)",
    "illop_b2",
    "illop_b3",
    "LDY $@,X",
    "LDA $@,X",
    "LDX $@,Y",
    "illop_b7",
    "CLV",
    "LDA $@,Y",
    "TSX",
    "illop_bb",
    "LDY $@,X",
    "LDA $@,X",
    "LDX $@,Y",
    "illop_bf",
    "CPY #$@",
    "CMP ($@,X)",
    "illop_c2",
    "illop_c3",
    "CPY $@",
    "CMP $@",
    "DEC $@",
    "illop_c7",
    "INY",
    "CMP #$@",
    "DEX",
    "illop_cb",
    "CPY $@",
    "CMP $@",
    "DEC $@",
    "illop_cf",
    "BNE $@",
    "CMP ($@,Y)",
    "illop_d2",
    "illop_d3",
    "illop_d4",
    "CMP $@,X",
    "DEC $@,X",
    "illop_d7",
    "CLD",
    "CMP $@,Y",
    "illop_da",
    "illop_db",
    "illop_dc",
    "CMP $@,X",
    "DEC $@,X",
    "illop_df",
    "CPX #$@",
    "SBC ($@,X)",
    "illop_e2",
    "illop_e3",
    "CPX $@",
    "SBC $@",
    "INC $@",
    "illop_e7",
    "INX",
    "SBC #$@",
    "NOP",
    "illop_eb",
    "CPX $@",
    "SBC $@",
    "INC $@",
    "illop_ef",
    "BEQ $@",
    "SBC ($@,Y)",
    "illop_f2",
    "illop_f3",
    "illop_f4",
    "SBC $@,X",
    "INC $@,X",
    "illop_f7",
    "SED",
    "SBC $@,Y",
    "illop_fa",
    "illop_fb",
    "illop_fc",
    "SBC $@,X",
    "INC $@,X",
    "illop_ff",
];
