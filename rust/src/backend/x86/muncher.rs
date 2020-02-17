use backend::x86::x86function::X86Function;
use backend::x86::x86instr::BinaryInstr::*;
use backend::x86::x86instr::JumpCond::*;
use backend::x86::x86instr::Operand::*;
use backend::x86::x86instr::UnaryInstr::*;
use backend::x86::x86instr::X86Instr;
use backend::x86::x86instr::X86Instr::*;
use backend::x86::x86instr::*;
use backend::x86::x86platform::X86Platform;
use backend::x86::x86prg::X86Prg;
use backend::x86::x86register::*;
use backend::Platform;
use ident::Ident;
use intermediate::tree;
use intermediate::tree::{BinOp, Exp, RelOp, Stm};
use std::collections::BTreeMap;
use std::mem;

pub struct Muncher {
    code: Vec<X86Instr>,
}

fn reg(i: Ident) -> Operand {
    Reg(X86Register::from(i))
}

impl Muncher {
    pub fn new(traced: &tree::Prg) -> X86Prg {
        Muncher { code: Vec::new() }.prg(traced)
    }

    fn prg(&mut self, prg: &tree::Prg) -> X86Prg {
        let mut functions = Vec::new();
        for m in &prg.functions {
            functions.push(self.function(m))
        }
        X86Prg {
            functions: functions,
        }
    }

    fn function(&mut self, m: &tree::Function) -> X86Function {
        self.code.clear();

        self.emit(Unary(PUSH, Reg(EBP)));
        self.emit(Binary(MOV, Reg(EBP), Reg(ESP)));
        self.emit(Binary(SUB, Reg(ESP), ImmFrameSize));

        let ebx_save = reg(Ident::new());
        let esi_save = reg(Ident::new());
        let edi_save = reg(Ident::new());
        self.emit(Binary(MOV, ebx_save.clone(), Reg(EBX)));
        self.emit(Binary(MOV, esi_save.clone(), Reg(ESI)));
        self.emit(Binary(MOV, edi_save.clone(), Reg(EDI)));

        for s in &m.body {
            self.stm(s)
        }

        self.emit(Binary(MOV, Reg(EAX), Reg(X86Register::from(m.ret))));
        self.emit(Binary(MOV, Reg(EBX), ebx_save));
        self.emit(Binary(MOV, Reg(ESI), esi_save));
        self.emit(Binary(MOV, Reg(EDI), edi_save));
        self.emit(Binary(MOV, Reg(ESP), Reg(EBP)));
        self.emit(Unary(POP, Reg(EBP)));
        self.emit(RET);

        let code = mem::replace(&mut self.code, Vec::new());
        X86Function::new(m.name.to_owned(), code)
    }

    fn stm(&mut self, s: &Stm) {
        match *s {
            Stm::Move(ref d, ref s) => match (self.lexp(d), self.exp(s)) {
                (Reg(r), Imm(0)) => self.emit(Binary(XOR, Reg(r.clone()), Reg(r))),
                (Mem(r), Mem(s)) => {
                    let i = Ident::new();
                    self.emit(Binary(MOV, reg(i), Mem(s)));
                    self.emit(Binary(MOV, Mem(r), reg(i)))
                }
                (r, s) => self.emit(Binary(MOV, r, s)),
            },
            Stm::Jump(Exp::Name(ref l), _) => self.emit(JMP(l.clone())),
            Stm::Jump(_, _) => panic!("not supported"),
            Stm::CJump(ref rel, ref e1, ref e2, ref lt, _) => {
                let cond = match *rel {
                    RelOp::EQ => E,
                    RelOp::NE => NE,
                    RelOp::LT => L,
                    RelOp::GT => G,
                    RelOp::LE => LE,
                    RelOp::GE => GE,
                    _ => panic!("relation not supported"),
                };
                match (self.exp(e1), self.exp(e2)) {
                    (Imm(n), r) => {
                        let i = Ident::new();
                        self.emit(Binary(MOV, reg(i), Imm(n)));
                        self.emit(Binary(CMP, reg(i), r));
                    }
                    (Mem(lm), Mem(rm)) => {
                        let i = Ident::new();
                        self.emit(Binary(MOV, reg(i), Mem(lm)));
                        self.emit(Binary(CMP, reg(i), Mem(rm)));
                    }
                    (l, r) => self.emit(Binary(CMP, l, r)),
                };
                self.emit(J(cond, lt.clone()));
            }
            Stm::Seq(ref stms) => {
                for s in stms {
                    self.stm(s)
                }
            }
            Stm::Label(ref l) => self.emit(Label(l.clone())),
        }
    }

    fn lexp(&mut self, e: &Exp) -> Operand {
        match *e {
            Exp::Temp(ref t) => reg(t.clone()),
            Exp::Mem(ref m) => self.effective_address(m),
            Exp::Param(ref n) => Mem(EffectiveAddress {
                base: Some(EBP),
                index_scale: None,
                displacement: (8 + 4 * n.clone()) as i32,
            }), //TODO: copy-paste
            _ => panic!("unexpected lexp"),
        }
    }

    fn effective_address(&mut self, e: &Exp) -> Operand {
        if let Ok(ea) = LinearCombination::munch(e).and_then(|l| l.into_effective_address()) {
            Operand::Mem(ea)
        } else {
            let o = self.exp(e);
            let r = X86Register::from(Ident::new());
            self.emit(Binary(MOV, Reg(r), o));
            Mem(EffectiveAddress {
                base: Some(r),
                index_scale: None,
                displacement: 0,
            })
        }
    }

    fn exp(&mut self, e: &Exp) -> Operand {
        // try linear combination in LEA first
        if let Ok(ea) = LinearCombination::munch(e).and_then(|l| l.into_effective_address()) {
            let n = if ea.base.is_none() { 0 } else { 1 }
                + if ea.index_scale.is_none() { 0 } else { 1 }
                + if 0 == ea.displacement { 0 } else { 1 };
            if n > 1 {
                let t = Ident::new();
                self.emit(Binary(LEA, reg(t), Mem(ea)));
                return reg(t);
            }
        }

        match *e {
            Exp::Const(ref n) => Imm(n.clone()),
            Exp::Name(_) => panic!("not supported"), //Sym(l.clone()),
            Exp::Temp(ref t) => reg(*t),
            Exp::Param(ref n) => Mem(EffectiveAddress {
                base: Some(EBP),
                index_scale: None,
                displacement: (8 + 4 * n) as i32,
            }),
            Exp::Mem(ref e) => self.effective_address(e),
            Exp::BinOp(ref o, ref e1, ref e2) => {
                let l = self.exp(e1);
                let r = self.exp(e2);
                let generic = |m: &mut Muncher, o: BinaryInstr, l: Operand, r: Operand| {
                    let t = Ident::new();
                    m.emit(Binary(MOV, reg(t), l));
                    m.emit(Binary(o, reg(t), r));
                    reg(t)
                };
                match *o {
                    BinOp::Plus => generic(self, ADD, l, r),
                    BinOp::Minus => generic(self, SUB, l, r),
                    BinOp::Mul => generic(self, IMUL, l, r),
                    BinOp::And => generic(self, AND, l, r),
                    BinOp::Or => generic(self, OR, l, r),
                    BinOp::LShift => generic(self, SHL, l, r),
                    BinOp::RShift => generic(self, SHR, l, r),
                    BinOp::ARShift => generic(self, SAR, l, r),
                    BinOp::Xor => generic(self, XOR, l, r),
                    BinOp::Div => match r {
                        Imm(2) => {
                            let t1 = Ident::new();
                            let t2 = Ident::new();
                            self.emit(Binary(MOV, reg(t2), l));
                            self.emit(Binary(MOV, reg(t1), reg(t2)));
                            self.emit(Binary(SHR, reg(t1), Imm(31)));
                            self.emit(Binary(ADD, reg(t2), reg(t1)));
                            self.emit(Binary(SAR, reg(t2), Imm(1)));
                            reg(t2)
                        }
                        Imm(n) => {
                            self.emit(Binary(MOV, Reg(EAX), l));
                            self.emit(Binary(MOV, Reg(EDX), Reg(EAX)));
                            self.emit(Binary(SAR, Reg(EDX), Imm(31)));
                            let t = Ident::new();
                            self.emit(Binary(MOV, reg(t), Imm(n)));
                            self.emit(Unary(IDIV, reg(t)));
                            let s = Ident::new();
                            self.emit(Binary(MOV, reg(s), Reg(EAX)));
                            reg(s)
                        }
                        r => {
                            self.emit(Binary(MOV, Reg(EAX), l));
                            self.emit(Binary(MOV, Reg(EDX), Reg(EAX)));
                            self.emit(Binary(SAR, Reg(EDX), Imm(31)));
                            self.emit(Unary(IDIV, r));
                            let s = Ident::new();
                            self.emit(Binary(MOV, reg(s), Reg(EAX)));
                            reg(s)
                        }
                    },
                }
            }
            Exp::Call(ref ef, ref args) => match **ef {
                Exp::Name(ref f) => {
                    let n = args.len();
                    for a in args.iter().rev() {
                        let o = self.exp(&a);
                        self.emit(Unary(PUSH, o))
                    }
                    let r = Ident::new();
                    self.emit(CALL(*f));
                    self.emit(Binary(MOV, reg(r), Reg(EAX)));
                    self.emit(Binary(
                        ADD,
                        Reg(ESP),
                        Imm((X86Platform::word_size() * n) as i32),
                    ));
                    reg(r)
                }
                _ => panic!("not implemented"),
            },
            Exp::ESeq(_, _) => panic!("not canonized"),
        }
    }

    fn emit(&mut self, instr: X86Instr) {
        self.code.push(instr)
    }
}

struct LinearCombination {
    constant: i32,
    coefficients: BTreeMap<X86Register, i32>,
}

impl LinearCombination {
    fn num(i: i32) -> LinearCombination {
        LinearCombination {
            constant: i,
            coefficients: BTreeMap::new(),
        }
    }

    fn var(t: X86Register) -> LinearCombination {
        let mut cs = BTreeMap::new();
        cs.insert(t, 1);
        LinearCombination {
            constant: 0,
            coefficients: cs,
        }
    }

    fn munch(e: &Exp) -> Result<LinearCombination, ()> {
        match *e {
            Exp::Const(ref n) => Ok(LinearCombination::num(*n)),
            Exp::Temp(ref t) => Ok(LinearCombination::var(X86Register::from(*t))),
            Exp::BinOp(ref o, ref e1, ref e2) => {
                let mut l1 = LinearCombination::munch(e1)?;
                let mut l2 = LinearCombination::munch(e2)?;
                match *o {
                    BinOp::Plus => l1.add(&l2)?,
                    BinOp::Mul => l1.mul(&l2)?,
                    BinOp::Minus => {
                        l2.mul(&LinearCombination::num(-1))?;
                        l1.add(&l2)?;
                    }
                    _ => return Err(()),
                }
                Ok(l1)
            }
            _ => Err(()),
        }
    }

    fn add(&mut self, other: &LinearCombination) -> Result<(), ()> {
        self.constant += other.constant;
        for (i, v) in other.coefficients.iter() {
            *self.coefficients.entry(*i).or_insert(0) += *v
        }
        Ok(())
    }

    fn mul(&mut self, other: &LinearCombination) -> Result<(), ()> {
        if self.coefficients.len() > 0 && other.coefficients.len() > 0 {
            return Err(());
        }
        let mul_constant = self.constant * other.constant;
        for (_, v) in self.coefficients.iter_mut() {
            *v *= other.constant
        }
        for (i, v) in other.coefficients.iter() {
            *self.coefficients.entry(*i).or_insert(*v) *= self.constant
        }
        self.constant = mul_constant;
        Ok(())
    }

    fn into_effective_address(self) -> Result<EffectiveAddress, ()> {
        match self.coefficients.len() {
            0 => Ok(EffectiveAddress {
                base: None,
                index_scale: None,
                displacement: self.constant,
            }),
            1 => {
                let (i, v) = self.coefficients.iter().next().unwrap();
                let scale = Scale::try_from(*v)?;
                Ok(EffectiveAddress {
                    base: None,
                    index_scale: Some((*i, scale)),
                    displacement: self.constant,
                })
            }
            2 => {
                let mut it = self.coefficients.iter();
                let (base, bv) = it.next().unwrap();
                let (index, iv) = it.next().unwrap();
                if *bv != 1 {
                    return Err(());
                }
                let scale = Scale::try_from(*iv)?;
                Ok(EffectiveAddress {
                    base: Some(*base),
                    index_scale: Some((*index, scale)),
                    displacement: self.constant,
                })
            }
            _ => Err(()),
        }
    }
}
