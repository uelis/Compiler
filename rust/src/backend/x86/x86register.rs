use backend::MachineReg;
use ident::Ident;
use std::convert::From;

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Debug)]
pub struct X86Register {
    pub number: usize,
}

pub const EAX: X86Register = X86Register { number: 0 };
pub const EBX: X86Register = X86Register { number: 1 };
pub const ECX: X86Register = X86Register { number: 2 };
pub const EDX: X86Register = X86Register { number: 3 };
pub const ESI: X86Register = X86Register { number: 4 };
pub const EDI: X86Register = X86Register { number: 5 };
pub const EBP: X86Register = X86Register { number: 6 };
pub const ESP: X86Register = X86Register { number: 7 };

const PHYSICAL_REGS: [X86Register; 8] = [EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP];
const GENERAL_PURPOSE_REGS: [X86Register; 6] = [EAX, EBX, ECX, EDX, ESI, EDI];

pub const PHYSICAL_REG_NAMES: [&str; 8] = ["eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp"];

pub static CALLEE_SAVE: [&X86Register; 3] = [&EBX, &ESI, &EDI];
pub static CALLER_SAVE: [&X86Register; 3] = [&EAX, &ECX, &EDX];

impl X86Register {
    pub fn new(n: usize) -> X86Register {
        X86Register { number: n }
    }
}

impl MachineReg for X86Register {
    fn is_physical(&self) -> bool {
        self.number < PHYSICAL_REGS.len()
    }

    fn physical_registers() -> &'static [X86Register] {
        &PHYSICAL_REGS
    }

    fn general_purpose_registers() -> &'static [X86Register] {
        &GENERAL_PURPOSE_REGS
    }
}

impl From<Ident> for X86Register {
    fn from(i: Ident) -> X86Register {
        X86Register::new(i.id + PHYSICAL_REGS.len())
    }
}
