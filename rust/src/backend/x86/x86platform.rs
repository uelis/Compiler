use backend::x86::x86function::X86Function;
use backend::x86::x86instr::X86Instr;
use backend::x86::x86prg::X86Prg;
use backend::x86::x86register::*;
use backend::Platform;
use intermediate::tree;

#[derive(Copy, Clone)]
pub struct X86Platform {}

impl Platform for X86Platform {
    type Reg = X86Register;
    type Instr = X86Instr;
    type Function = X86Function;
    type Prg = X86Prg;

    fn word_size() -> usize {
        4
    }

    fn code_gen(prg: &tree::Prg) -> X86Prg {
        ::backend::x86::muncher::Muncher::new(prg)
    }
}
