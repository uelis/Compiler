use ident::Label;
use intermediate::tree;
use std::fmt::{Debug, Display};
use std::hash::Hash;

pub trait MachineReg: Copy + Eq + Hash {
    fn is_physical(&self) -> bool;
    fn physical_registers() -> &'static [Self];
    fn general_purpose_registers() -> &'static [Self];
}

pub trait MachineInstr<Reg> {
    type UseIterator: Iterator<Item = Reg>;
    type DefIterator: Iterator<Item = Reg>;
    type TargetIterator: Iterator<Item = Label>;

    fn uses(&self) -> Self::UseIterator;

    fn defs(&self) -> Self::DefIterator;

    fn is_fall_through(&self) -> bool;

    fn jumps(&self) -> Self::TargetIterator;

    fn is_move_between_temps(&self) -> Option<(Reg, Reg)>;

    fn is_label(&self) -> Option<Label>;

    //noinspection RsAnonymousParameter
    fn rename(&mut self, sigma: &dyn Fn(Reg) -> Reg);
}

pub trait MachineFunction<Reg, A: MachineInstr<Reg>>: Debug {
    fn name(&self) -> &Label;
    fn body(&self) -> &Vec<A>;
    fn spill(&mut self, to_spill: &[Reg]);
    fn rename(&mut self, sigma: &dyn Fn(Reg) -> Reg);
}

pub trait MachinePrg<Reg, A: MachineInstr<Reg>, F: MachineFunction<Reg, A>>:
    Debug + Display
{
    fn functions(&self) -> &Vec<F>;
    fn functions_mut(&mut self) -> &mut Vec<F>;
}

pub trait Platform {
    type Reg: MachineReg;
    type Instr: MachineInstr<Self::Reg>;
    type Function: MachineFunction<Self::Reg, Self::Instr>;
    type Prg: MachinePrg<Self::Reg, Self::Instr, Self::Function>;

    fn word_size() -> usize;
    fn code_gen(c: &tree::Prg) -> Self::Prg;
}
