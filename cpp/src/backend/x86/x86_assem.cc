#include <iostream>

#include "backend/x86/x86_function.h"
#include "backend/x86/x86_instr.h"
#include "backend/x86/x86_prg.h"
#include "backend/x86/x86_target.h"

namespace mjc {

using R = typename X86Target::Reg;

std::ostream &operator<<(std::ostream &os, const UnaryInstrKind &cond) {
  switch (cond) {
    case NEG:
      return os << "NEG";
    case NOT:
      return os << "NOT";
    case INC:
      return os << "INC";
    case DEC:
      return os << "DEC";
    case IDIV:
      return os << "IDIV";
    case PUSH:
      return os << "PUSH";
    case POP:
      return os << "POP";
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const BinaryInstrKind &cond) {
  switch (cond) {
    case MOV:
      return os << "MOV";
    case ADD:
      return os << "ADD";
    case SUB:
      return os << "SUB";
    case SHL:
      return os << "SHL";
    case SHR:
      return os << "SHR";
    case SAL:
      return os << "SAL";
    case SAR:
      return os << "SAR";
    case AND:
      return os << "AND";
    case OR:
      return os << "OR";
    case XOR:
      return os << "XOR";
    case TEST:
      return os << "TEST";
    case CMP:
      return os << "CMP";
    case LEA:
      return os << "LEA";
    case IMUL:
      return os << "IMUL";
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const JInstr::Kind &cond) {
  switch (cond) {
    case JInstr::E:
      return os << "E";
    case JInstr::NE:
      return os << "NE";
    case JInstr::L:
      return os << "L";
    case JInstr::LE:
      return os << "LE";
    case JInstr::G:
      return os << "GE";
    case JInstr::GE:
      return os << "GE";
    case JInstr::Z:
      return os << "Z";
  }
  return os;
}

std::ostream &Assem(std::ostream &os, X86Register reg) {
  if (reg.number < NUMBER_OF_REGS) {
    return os << REG_NAMES[reg.number];
  } else {
    return os << "t" << reg.number;
  }
}

// TODO: this breaks abstraction
std::ostream &Assem(std::ostream &os, const X86Function &f, const Operand &op) {
  switch (op.kind_) {
    case Operand::IMM:
      return os << op.imms_[0];
    case Operand::MEM_BASE:
      os << "DWORD PTR [ ";
      Assem(os, op.regs_[0]);
      if (op.imms_.size() == 1) {
        os << " + " << op.imms_[0];
      }
      return os << " ]";
    case Operand::MEM_INDEX:
      os << "DWORD PTR [" << op.imms_[0];
      os << " * ";
      Assem(os, op.regs_[0]);
      if (op.imms_.size() == 2) {
        os << " + " << op.imms_[1];
      }
      return os << "]";
    case Operand::MEM_BASE_INDEX:
      os << "DWORD PTR [";
      Assem(os, op.regs_[0]);
      os << " + " << op.imms_[0] << " * ";
      Assem(os, op.regs_[1]);
      if (op.imms_.size() == 2) {
        os << " + " << op.imms_[1];
      }
      return os << "]";
    case Operand::REG:
      return Assem(os, op.regs_[0]);
    case Operand::FRAMESIZE:
      return os << f.GetFrameSize();
  }
  return os;
}

class AssemInstrVisitor : public X86InstrVisitor {
 public:
  AssemInstrVisitor(std::ostream &os, X86Function &function)
      : os_(os), function_(function) {}

  void Visit(UnaryInstr &i) {
    os_ << i.kind << " ";
    Assem(os_, function_, i.src);
  }
  void Visit(BinaryInstr &i) {
    os_ << i.kind << " ";
    Assem(os_, function_, i.dst) << ", ";
    Assem(os_, function_, i.src);
  }
  void Visit(LabelInstr &i) { os_ << i.label << ":"; }
  void Visit(CallInstr &i) { os_ << "CALL " << i.target; }
  void Visit(JmpInstr &i) { os_ << "JMP " << i.target; }
  void Visit(JInstr &i) { os_ << "J" << i.cond << " " << i.target; }
  void Visit(RetInstr &i) { os_ << "RET"; }

 private:
  std::ostream &os_;
  X86Function &function_;
};

void AssemFunction(std::ostream &os, X86Function &f) {
  os << f.GetName() << ":" << std::endl;
  AssemInstrVisitor iv(os, f);
  for (auto &i : f.GetBody()) {
    os << "  ";
    i->accept(iv);
    os << std::endl;
  }
}

void AssemPrg(std::ostream &os, X86Prg &p) {
  os << ".intel_syntax noprefix" << std::endl;
  os << ".global Lmain" << std::endl;
  for (auto &f : p.functions) {
    AssemFunction(os, *f);
    os << std::endl;
  }
}

std::ostream &operator<<(std::ostream &os, X86Function &f) {
  AssemFunction(os, f);
  return os;
}

std::ostream &operator<<(std::ostream &os, X86Prg &p) {
  AssemPrg(os, p);
  return os;
}
}  // namespace mjc
