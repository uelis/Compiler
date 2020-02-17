use backend;
use backend::x86::x86register::*;
use ident::Label;

#[derive(Debug, Copy, Clone)]
pub enum UnaryInstr {
    PUSH,
    POP,
    NEG,
    NOT,
    INC,
    DEC,
    IDIV,
}

#[derive(Debug, Copy, Clone)]
pub enum BinaryInstr {
    MOV,
    ADD,
    SUB,
    SHL,
    SHR,
    SAL,
    SAR,
    AND,
    OR,
    XOR,
    TEST,
    CMP,
    LEA,
    IMUL,
}

#[derive(Debug, Copy, Clone)]
pub enum JumpCond {
    E,
    NE,
    L,
    LE,
    G,
    GE,
    Z,
}

#[derive(Debug, Copy, Clone)]
pub enum Scale {
    S1,
    S2,
    S4,
    S8,
}

#[derive(Debug, Copy, Clone)]
pub struct EffectiveAddress {
    pub base: Option<X86Register>,
    pub index_scale: Option<(X86Register, Scale)>,
    pub displacement: i32,
}

#[derive(Debug, Clone)]
pub enum Operand {
    Imm(i32),
    Reg(X86Register),
    Mem(EffectiveAddress),
    ImmFrameSize, //    Sym(Ident)
}

#[derive(Debug)]
pub enum X86Instr {
    Unary(UnaryInstr, Operand),
    Binary(BinaryInstr, Operand, Operand),
    Label(Label),
    JMP(Label),
    // only static calls needed for now
    CALL(Label),
    J(JumpCond, Label),
    RET,
}

impl Scale {
    pub fn try_from(x: i32) -> Result<Scale, ()> {
        match x {
            1 => Ok(Scale::S1),
            2 => Ok(Scale::S2),
            4 => Ok(Scale::S4),
            8 => Ok(Scale::S8),
            _ => Err(()),
        }
    }
}

impl EffectiveAddress {
    fn add_uses(&self, uses: &mut Vec<X86Register>) {
        self.base.map(|t| uses.push(t));
        match self.index_scale {
            None => (),
            Some((i, _)) => uses.push(i),
        }
    }

    fn rename(&mut self, sigma: &dyn Fn(X86Register) -> X86Register) {
        self.base = match self.base {
            None => None,
            Some(t) => Some(sigma(t)),
        };
        match self.index_scale {
            None => (),
            Some((i, s)) => self.index_scale = Some((sigma(i), s)),
        }
    }
}

impl Operand {
    fn add_uses(&self, uses: &mut Vec<X86Register>) {
        use self::Operand::*;
        match *self {
            Imm(_) => (),
            Reg(t) => uses.push(t),
            Mem(ref ea) => ea.add_uses(uses),
            ImmFrameSize => (),
        }
    }

    fn rename(&mut self, sigma: &dyn Fn(X86Register) -> X86Register) {
        use self::Operand::*;
        match *self {
            Imm(_) => (),
            Reg(t) => *self = Reg(sigma(t)),
            Mem(ref mut ea) => ea.rename(sigma),
            ImmFrameSize => (),
        }
    }
}

pub struct TargetIterator(Option<Label>);

impl Iterator for TargetIterator {
    type Item = Label;
    fn next(&mut self) -> Option<Label> {
        match *self {
            TargetIterator(None) => None,
            TargetIterator(Some(l)) => {
                *self = TargetIterator(None);
                Some(l)
            }
        }
    }
}

impl backend::MachineInstr<X86Register> for X86Instr {
    type UseIterator = ::std::vec::IntoIter<X86Register>;
    type DefIterator = ::std::vec::IntoIter<X86Register>;
    type TargetIterator = TargetIterator;

    fn uses(&self) -> Self::UseIterator {
        use self::BinaryInstr::*;
        use self::Operand::*;
        use self::UnaryInstr::*;
        use self::X86Instr::*;
        let mut uses = Vec::with_capacity(4);
        match *self {
            Unary(PUSH, ref src) => src.add_uses(&mut uses),
            Unary(POP, _) => (),
            Unary(NEG, ref src) => src.add_uses(&mut uses),
            Unary(NOT, ref src) => src.add_uses(&mut uses),
            Unary(INC, ref src) => src.add_uses(&mut uses),
            Unary(DEC, ref src) => src.add_uses(&mut uses),
            Unary(IDIV, ref src) => {
                src.add_uses(&mut uses);
                uses.push(EAX);
                uses.push(EDX);
            }
            Binary(XOR, Reg(ref t), Reg(ref s)) => {
                if s != t {
                    uses.push(*s)
                    // TODO: t auch
                }
            }
            Binary(MOV, Reg(_), ref src) => src.add_uses(&mut uses),
            Binary(LEA, Reg(_), ref src) => src.add_uses(&mut uses),
            Binary(_, ref dst, ref src) => {
                dst.add_uses(&mut uses);
                src.add_uses(&mut uses)
            }
            Label(_) => (),
            JMP(_) => (),
            CALL(_) => (),
            J(_, _) => (),
            RET => {
                uses.extend(CALLEE_SAVE.iter().cloned());
                uses.push(EAX)
            }
        }
        uses.dedup();
        uses.into_iter()
    }

    fn defs(&self) -> Self::DefIterator {
        use self::BinaryInstr::*;
        use self::Operand::*;
        use self::UnaryInstr::*;
        use self::X86Instr::*;
        let mut defs = Vec::with_capacity(2);
        match *self {
            Unary(PUSH, _) => (),
            Unary(POP, Reg(t)) => defs.push(t),
            Unary(NEG, Reg(t)) => defs.push(t),
            Unary(NOT, Reg(t)) => defs.push(t),
            Unary(INC, Reg(t)) => defs.push(t),
            Unary(DEC, Reg(t)) => defs.push(t),
            Unary(IDIV, _) => {
                defs.push(EAX);
                defs.push(EDX);
            }
            Unary(_, _) => (),
            Binary(CMP, _, _) => (),
            Binary(TEST, _, _) => (),
            Binary(_, Reg(t), _) => defs.push(t),
            Binary(_, _, _) => (),
            Label(_) => (),
            JMP(_) => (),
            CALL(_) => {
                defs.extend(CALLER_SAVE.iter().cloned());
                defs.push(EAX)
            }
            J(_, _) => (),
            RET => (),
        }
        defs.into_iter()
    }

    fn is_fall_through(&self) -> bool {
        match *self {
            X86Instr::JMP(_) => false,
            X86Instr::RET => false,
            _ => true,
        }
    }

    fn jumps(&self) -> Self::TargetIterator {
        match *self {
            X86Instr::J(_, l) => TargetIterator(Some(l)),
            X86Instr::JMP(l) => TargetIterator(Some(l)),
            _ => TargetIterator(None),
        }
    }

    fn is_move_between_temps(&self) -> Option<(X86Register, X86Register)> {
        use self::BinaryInstr::*;
        use self::Operand::*;
        use self::X86Instr::*;
        match *self {
            Binary(MOV, Reg(d), Reg(s)) => Some((d, s)),
            _ => None,
        }
    }

    fn is_label(&self) -> Option<Label> {
        match *self {
            X86Instr::Label(l) => Some(l),
            _ => None,
        }
    }

    fn rename(&mut self, sigma: &dyn Fn(X86Register) -> X86Register) {
        use self::X86Instr::*;
        match *self {
            Unary(_, ref mut o) => o.rename(sigma),
            Binary(_, ref mut o1, ref mut o2) => {
                o1.rename(sigma);
                o2.rename(sigma)
            }
            Label(_) => (),
            J(_, _) => (),
            CALL(_) => (),
            JMP(_) => (),
            RET => (),
        }
    }
}
