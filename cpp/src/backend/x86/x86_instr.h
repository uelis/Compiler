//
// x86 machine instruction
//
#ifndef MJC_BACKEND_X86INSTR_H
#define MJC_BACKEND_X86INSTR_H

#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "backend/x86/x86_function.h"
#include "backend/x86/x86_registers.h"
#include "intermediate/names.h"

namespace mjc {

class Operand {
 public:
  enum Kind { IMM, MEM_BASE, MEM_INDEX, MEM_BASE_INDEX, REG, FRAMESIZE };
  enum Scale { S1, S2, S4, S8 };

  Operand() = delete;
  Operand(X86Register reg);

  bool IsImm() const;
  // defined only if IsImm
  std::int32_t GetImm() const;
  bool IsReg() const;
  // defined only if IsReg
  X86Register GetReg() const;
  bool IsMem() const;

  static std::optional<Scale> ToScale(int i);
  static int FromScale(Scale s);

  // convenience builder functions
  static Operand Imm(std::int32_t value);
  static Operand Reg(X86Register r);
  static Operand Mem(X86Register base);
  static Operand Mem(X86Register base, std::int32_t disp);
  static Operand Mem(Scale scale, X86Register index);
  static Operand Mem(Scale scale, X86Register index, std::int32_t disp);
  static Operand Mem(X86Register base, Scale scale, X86Register index);
  static Operand Mem(X86Register base, Scale scale, X86Register index,
                     std::int32_t disp);
  static Operand FrameSize();

  Kind GetKind() const;
  const std::vector<X86Register>& GetRegs() const;
  void rename(std::function<X86Register(X86Register)>& sigma);

  friend std::ostream& Assem(std::ostream& os, const X86Function& f,
                             const Operand& op);

 private:
  explicit Operand(Kind kind, std::vector<X86Register> regs,
                   std::vector<std::int32_t> imms);

  Kind kind_;
  std::vector<X86Register> regs_;  // not const because of renaming
  std::vector<std::int32_t> imms_;
};

class X86InstrVisitor;

// Base class for composite that represents instructions.
// Instructions would be represented more efficiently without the many
// virtual functions.
class X86Instr {
 public:
  virtual std::vector<X86Register> Uses() const = 0;
  virtual std::vector<X86Register> Defs() const = 0;
  virtual bool IsFallThrough() const = 0;
  virtual std::optional<Label> IsLabel() const = 0;
  virtual std::vector<Label> Jumps() const = 0;
  virtual std::optional<std::pair<X86Register, X86Register>>
  IsMoveBetweenTemps() const = 0;
  virtual void rename(std::function<X86Register(X86Register)>& sigma) = 0;

  virtual void accept(X86InstrVisitor& visitor) = 0;
  virtual ~X86Instr(){};
};

enum UnaryInstrKind { PUSH, POP, NEG, NOT, INC, DEC, IDIV };

class UnaryInstr : public X86Instr {
 public:
  const UnaryInstrKind kind;
  Operand src;

  UnaryInstr(UnaryInstrKind kind, Operand src)
      : kind(std::move(kind)), src(std::move(src)) {}

  virtual std::vector<X86Register> Uses() const;
  virtual std::vector<X86Register> Defs() const;
  virtual bool IsFallThrough() const;
  virtual std::optional<Label> IsLabel() const;
  virtual std::vector<Label> Jumps() const;
  virtual std::optional<std::pair<X86Register, X86Register>>
  IsMoveBetweenTemps() const;
  virtual void rename(std::function<X86Register(X86Register)>& sigma);

  virtual void accept(X86InstrVisitor& visitor);

 private:
};

enum BinaryInstrKind {
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
};

class BinaryInstr : public X86Instr {
 public:
  const BinaryInstrKind kind;
  Operand src;
  Operand dst;

  BinaryInstr(BinaryInstrKind kind, Operand dst, Operand src)
      : kind(std::move(kind)), src(std::move(src)), dst(std::move(dst)) {}

  virtual std::vector<X86Register> Uses() const;
  virtual std::vector<X86Register> Defs() const;
  virtual bool IsFallThrough() const;
  virtual std::optional<Label> IsLabel() const;
  virtual std::vector<Label> Jumps() const;
  virtual std::optional<std::pair<X86Register, X86Register>>
  IsMoveBetweenTemps() const;
  virtual void rename(std::function<X86Register(X86Register)>& sigma);

  virtual void accept(X86InstrVisitor& visitor);
};

class LabelInstr : public X86Instr {
 public:
  const Label label;

  LabelInstr(Label l) : label(std::move(l)){};

  virtual std::vector<X86Register> Uses() const;
  virtual std::vector<X86Register> Defs() const;
  virtual bool IsFallThrough() const;
  virtual std::optional<Label> IsLabel() const;
  virtual std::vector<Label> Jumps() const;
  virtual std::optional<std::pair<X86Register, X86Register>>
  IsMoveBetweenTemps() const;
  virtual void rename(std::function<X86Register(X86Register)>& sigma);

  virtual void accept(X86InstrVisitor& visitor);
};

class CallInstr : public X86Instr {
 public:
  const Label target;
  CallInstr(Label l) : target(std::move(l)){};

  virtual std::vector<X86Register> Uses() const;
  virtual std::vector<X86Register> Defs() const;
  virtual bool IsFallThrough() const;
  virtual std::optional<Label> IsLabel() const;
  virtual std::vector<Label> Jumps() const;
  virtual std::optional<std::pair<X86Register, X86Register>>
  IsMoveBetweenTemps() const;
  virtual void rename(std::function<X86Register(X86Register)>& sigma);

  virtual void accept(X86InstrVisitor& visitor);
};

class JmpInstr : public X86Instr {
 public:
  const Label target;

  JmpInstr(Label l) : target(std::move(l)){};

  virtual std::vector<X86Register> Uses() const;
  virtual std::vector<X86Register> Defs() const;
  virtual bool IsFallThrough() const;
  virtual std::optional<Label> IsLabel() const;
  virtual std::vector<Label> Jumps() const;
  virtual std::optional<std::pair<X86Register, X86Register>>
  IsMoveBetweenTemps() const;
  virtual void rename(std::function<X86Register(X86Register)>& sigma);

  virtual void accept(X86InstrVisitor& visitor);
};

class JInstr : public X86Instr {
 public:
  enum Kind { E, NE, L, LE, G, GE, Z };
  const Kind cond;
  const Label target;

  JInstr(Kind cond, Label l) : cond(cond), target(std::move(l)){};

  virtual std::vector<X86Register> Uses() const;
  virtual std::vector<X86Register> Defs() const;
  virtual bool IsFallThrough() const;
  virtual std::optional<Label> IsLabel() const;
  virtual std::vector<Label> Jumps() const;
  virtual std::optional<std::pair<X86Register, X86Register>>
  IsMoveBetweenTemps() const;
  virtual void rename(std::function<X86Register(X86Register)>& sigma);

  virtual void accept(X86InstrVisitor& visitor);
};

class RetInstr : public X86Instr {
 public:
  virtual std::vector<X86Register> Uses() const;
  virtual std::vector<X86Register> Defs() const;
  virtual bool IsFallThrough() const;
  virtual std::optional<Label> IsLabel() const;
  virtual std::vector<Label> Jumps() const;
  virtual std::optional<std::pair<X86Register, X86Register>>
  IsMoveBetweenTemps() const;
  virtual void rename(std::function<X86Register(X86Register)>& sigma);

  virtual void accept(X86InstrVisitor& visitor);
};

class X86InstrVisitor {
 public:
  virtual void Visit(UnaryInstr& i) = 0;
  virtual void Visit(BinaryInstr& i) = 0;
  virtual void Visit(LabelInstr& i) = 0;
  virtual void Visit(CallInstr& i) = 0;
  virtual void Visit(JmpInstr& i) = 0;
  virtual void Visit(JInstr& i) = 0;
  virtual void Visit(RetInstr& i) = 0;
};
}  // namespace mjc

#endif
