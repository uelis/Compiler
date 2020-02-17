use backend::x86::x86function::X86Function;
use backend::x86::x86instr;
use backend::x86::x86prg::X86Prg;
use backend::x86::x86register::*;
use std::fmt::{Debug, Display, Error, Formatter};

impl Display for x86instr::UnaryInstr {
    fn fmt(&self, fmt: &mut Formatter) -> Result<(), Error> {
        Debug::fmt(self, fmt)
    }
}

impl Display for x86instr::BinaryInstr {
    fn fmt(&self, fmt: &mut Formatter) -> Result<(), Error> {
        Debug::fmt(self, fmt)
    }
}

impl Display for x86instr::JumpCond {
    fn fmt(&self, fmt: &mut Formatter) -> Result<(), Error> {
        Debug::fmt(self, fmt)
    }
}

impl Display for x86instr::Scale {
    fn fmt(&self, fmt: &mut Formatter) -> Result<(), Error> {
        use self::x86instr::Scale::*;
        match *self {
            S1 => write!(fmt, "1"),
            S2 => write!(fmt, "2"),
            S4 => write!(fmt, "4"),
            S8 => write!(fmt, "8"),
        }
    }
}

impl Display for X86Register {
    fn fmt(&self, fmt: &mut Formatter) -> Result<(), Error> {
        if self.number < PHYSICAL_REG_NAMES.len() {
            write!(fmt, "{}", PHYSICAL_REG_NAMES[self.number])
        } else {
            write!(fmt, "t{}", self.number)
        }
    }
}

impl Display for x86instr::EffectiveAddress {
    fn fmt(&self, fmt: &mut Formatter) -> Result<(), Error> {
        match (self.base, self.index_scale) {
            (None, None) => write!(fmt, "{}", self.displacement),
            (None, Some((i, s))) => write!(fmt, "{}*{} + {}", i, s, self.displacement),
            (Some(b), None) => write!(fmt, "{} + {}", b, self.displacement),
            (Some(b), Some((i, s))) => write!(fmt, "{} + {}*{} + {}", b, i, s, self.displacement),
        }
    }
}

impl Display for x86instr::Operand {
    fn fmt(&self, fmt: &mut Formatter) -> Result<(), Error> {
        use self::x86instr::Operand::*;
        match *self {
            Imm(n) => write!(fmt, "{}", n),
            Reg(i) => write!(fmt, "{}", i),
            Mem(i) => write!(fmt, "DWORD PTR [{}]", i),
            ImmFrameSize => write!(fmt, "frame_size"),
        }
    }
}

impl x86instr::Operand {
    fn display(&self, fmt: &mut Formatter, function: &X86Function) -> Result<(), Error> {
        use self::x86instr::Operand::*;
        match *self {
            Imm(n) => write!(fmt, "{}", n),
            Reg(i) => write!(fmt, "{}", i),
            Mem(i) => write!(fmt, "DWORD PTR [{}]", i),
            ImmFrameSize => write!(fmt, "{}", function.size()),
        }
    }
}

impl x86instr::X86Instr {
    pub fn display(&self, fmt: &mut Formatter, function: &X86Function) -> Result<(), Error> {
        use self::x86instr::X86Instr::*;
        match *self {
            Unary(i, ref o) => {
                write!(fmt, "{} ", i)?;
                o.display(fmt, function)
            }
            Binary(b, ref o1, ref o2) => {
                write!(fmt, "{} ", b)?;
                o1.display(fmt, function)?;
                write!(fmt, " , ")?;
                o2.display(fmt, function)
            }
            Label(ref l) => write!(fmt, "{}:", l),
            JMP(ref l) => write!(fmt, "JMP {}", l),
            CALL(ref l) => write!(fmt, "CALL {}", l),
            J(ref c, ref l) => write!(fmt, "J{} {}", c, l),
            RET => write!(fmt, "RET"),
        }
    }
}

impl Display for X86Function {
    fn fmt(&self, fmt: &mut Formatter) -> Result<(), Error> {
        use backend::MachineFunction;
        writeln!(fmt, "{}:", self.name())?;
        for i in self.body() {
            i.display(fmt, &self)?;
            writeln!(fmt)?
        }
        Ok(())
    }
}

impl Display for X86Prg {
    fn fmt(&self, fmt: &mut Formatter) -> Result<(), Error> {
        writeln!(fmt, "  .intel_syntax noprefix")?;
        writeln!(fmt, "  .global Lmain")?;
        for f in &self.functions {
            writeln!(fmt, "{}\n", f)?
        }
        Ok(())
    }
}
