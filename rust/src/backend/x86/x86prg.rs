use backend::x86::x86function::X86Function;
use backend::x86::x86instr::X86Instr;
use backend::x86::x86register::X86Register;
use backend::MachinePrg;

#[derive(Debug)]
pub struct X86Prg {
    pub functions: Vec<X86Function>,
}

impl MachinePrg<X86Register, X86Instr, X86Function> for X86Prg {
    fn functions(&self) -> &Vec<X86Function> {
        &self.functions
    }

    fn functions_mut(&mut self) -> &mut Vec<X86Function> {
        &mut self.functions
    }
}
