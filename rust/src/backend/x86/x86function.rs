use backend;
use backend::x86::x86instr::BinaryInstr::*;
use backend::x86::x86instr::EffectiveAddress;
use backend::x86::x86instr::Operand;
use backend::x86::x86instr::Operand::*;
use backend::x86::x86instr::X86Instr;
use backend::x86::x86instr::X86Instr::*;
use backend::x86::x86register::*;
use backend::MachineInstr;
use ident::{Ident, Label};
use std::collections::HashMap;
use std::mem;

#[derive(Debug)]
pub struct X86Function {
    pub name: Label,
    pub body: Vec<X86Instr>,
    locals_in_stack: usize,
}

impl X86Function {
    pub fn new(name: Label, body: Vec<X86Instr>) -> X86Function {
        X86Function {
            name: name,
            body: body,
            locals_in_stack: 0,
        }
    }

    pub fn add_local_on_stack(&mut self) -> Operand {
        self.locals_in_stack += 1;
        Operand::Mem(EffectiveAddress {
            base: Some(EBP),
            index_scale: None,
            displacement: -4 * (self.locals_in_stack as i32),
        })
    }

    pub fn size(&self) -> usize {
        self.locals_in_stack * 4
    }
}

impl backend::MachineFunction<X86Register, X86Instr> for X86Function {
    fn name(&self) -> &Label {
        &self.name
    }

    fn body(&self) -> &Vec<X86Instr> {
        &self.body
    }

    fn spill(&mut self, to_spill: &[X86Register]) {
        let mut spills = HashMap::new();

        for t in to_spill {
            spills.insert(*t, self.add_local_on_stack());
        }

        let mut fresh_idents = HashMap::new();

        let new_body = Vec::with_capacity(self.body.len());
        let body = mem::replace(&mut self.body, new_body);
        for mut i in body {
            fresh_idents.clear();
            match i.is_move_between_temps() {
                Some((dst, src)) if !(spills.contains_key(&dst) && spills.contains_key(&src)) => {
                    self.body.push(Binary(
                        MOV,
                        spills.get(&dst).unwrap_or(&Reg(dst)).clone(),
                        spills.get(&src).unwrap_or(&Reg(src)).clone(),
                    ));
                }
                _ => {
                    let mut loads = Vec::new();
                    for u in i.uses() {
                        if let Some(umem) = spills.get(&u) {
                            let uf = fresh_idents
                                .entry(u)
                                .or_insert_with(&|| X86Register::from(Ident::new()));
                            loads.push(Binary(MOV, Reg(*uf), umem.clone()));
                        }
                    }
                    let mut saves = Vec::new();
                    for d in i.defs() {
                        if let Some(dmem) = spills.get(&d) {
                            let df = fresh_idents
                                .entry(d)
                                .or_insert_with(&|| X86Register::from(Ident::new()));
                            saves.push(Binary(MOV, dmem.clone(), Reg(*df)));
                        }
                    }
                    self.body.extend(loads);
                    i.rename(&|t| *fresh_idents.get(&t).unwrap_or(&t));
                    self.body.push(i);
                    self.body.extend(saves);
                }
            }
        }
    }

    fn rename(&mut self, sigma: &dyn Fn(X86Register) -> X86Register) {
        let n = self.body.len();
        let body = mem::replace(&mut self.body, Vec::with_capacity(n));
        for mut i in body {
            i.rename(sigma);
            match i.is_move_between_temps() {
                Some((d, s)) if d == s => (),
                _ => self.body.push(i),
            }
        }
    }
}
