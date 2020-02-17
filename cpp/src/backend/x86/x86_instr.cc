#include "backend/x86/x86_instr.h"

#include <algorithm>

#include "backend/x86/x86_target.h"

namespace mjc {

Operand::Operand(X86Register reg) : kind_{REG}, regs_{reg} {}

Operand::Operand(Kind kind, std::vector<X86Register> regs,
                 std::vector<std::int32_t> imms)
    : kind_{kind}, regs_{std::move(regs)}, imms_{std::move(imms)} {}

bool Operand::IsImm() const { return kind_ == IMM; }

// defined only if IsImm
std::int32_t Operand::GetImm() const { return imms_[0]; }

bool Operand::IsReg() const { return kind_ == REG; }

// defined only if IsReg
X86Register Operand::GetReg() const { return regs_[0]; }

bool Operand::IsMem() const {
  return kind_ == MEM_BASE || kind_ == MEM_INDEX || kind_ == MEM_BASE_INDEX;
}

std::optional<Operand::Scale> Operand::ToScale(int i) {
  switch (i) {
  case 1:
    return Operand::S1;
  case 2:
    return Operand::S2;
  case 4:
    return Operand::S4;
  case 8:
    return Operand::S8;
  default:
    return std::nullopt;
  }
}

int Operand::FromScale(Scale s) {
  switch (s) {
  case S1:
    return 1;
  case S2:
    return 2;
  case S4:
    return 4;
  case S8:
    return 8;
  default:
    assert(false);
    abort();
  }
}

// convenience builder functions
Operand Operand::Imm(std::int32_t value) { return Operand(IMM, {}, {value}); }

Operand Operand::Reg(X86Register r) { return Operand(REG, {r}, {}); }

Operand Operand::Mem(X86Register base) { return Operand(MEM_BASE, {base}, {}); }

Operand Operand::Mem(X86Register base, std::int32_t disp) {
  return Operand(MEM_BASE, {base}, {disp});
}

Operand Operand::Mem(Scale scale, X86Register index) {
  return Operand(MEM_INDEX, {index}, {FromScale(scale)});
}

Operand Operand::Mem(Scale scale, X86Register index, std::int32_t disp) {
  return Operand(MEM_INDEX, {index}, {FromScale(scale), disp});
}

Operand Operand::Mem(X86Register base, Scale scale, X86Register index) {
  return Operand(MEM_BASE_INDEX, {base, index}, {FromScale(scale)});
}

Operand Operand::Mem(X86Register base, Scale scale, X86Register index,
                     std::int32_t disp) {
  return Operand(MEM_BASE_INDEX, {base, index}, {FromScale(scale), disp});
}

Operand Operand::FrameSize() { return Operand(FRAMESIZE, {}, {}); }

Operand::Kind Operand::GetKind() const { return kind_; }

const std::vector<X86Register> &Operand::GetRegs() const { return regs_; }

void Operand::rename(std::function<X86Register(X86Register)> &sigma) {
  std::transform(regs_.begin(), regs_.end(), regs_.begin(), sigma);
}

void AddRegs(std::vector<X86Register> &target, const Operand &o) {
  auto src = o.GetRegs();
  target.insert(target.end(), src.begin(), src.end());
}

std::vector<X86Register> UnaryInstr::Uses() const {
  switch (kind) {
  case NEG:
  case NOT:
  case INC:
  case DEC:
  case PUSH:
    return src.GetRegs();
  case IDIV: {
    std::vector<X86Register> uses(4);
    AddRegs(uses, src);
    uses.push_back(EAX);
    uses.push_back(EDX);
    return uses;
  }
  case POP:
    return {};
  };
  return {};
}
std::vector<X86Register> BinaryInstr::Uses() const {
  switch (kind) {
  case XOR:
    if (src.IsReg() && dst.IsReg() && src.GetReg() == dst.GetReg())
      return {};
    break;
  case LEA:
    return src.GetRegs();
  case MOV:
    if (dst.IsReg()) {
      return src.GetRegs();
    }
    break;
  default:
    break;
  }
  std::vector<X86Register> uses(4);
  AddRegs(uses, src);
  AddRegs(uses, dst);
  return uses;
}
std::vector<X86Register> LabelInstr::Uses() const { return {}; }
std::vector<X86Register> CallInstr::Uses() const { return {}; }
std::vector<X86Register> JmpInstr::Uses() const { return {}; }
std::vector<X86Register> JInstr::Uses() const { return {}; }
std::vector<X86Register> RetInstr::Uses() const {
  std::vector<X86Register> uses(std::begin(CALLEE_SAVE), std::end(CALLEE_SAVE));
  uses.push_back(EAX);
  return uses;
}

std::vector<X86Register> UnaryInstr::Defs() const {
  switch (kind) {
  case NEG:
  case NOT:
  case INC:
  case DEC:
  case POP:
    if (src.IsReg()) {
      return src.GetRegs();
    } else {
      return {};
    }
  case IDIV:
    return {EAX, EDX};
  case PUSH:
  default:
    return {};
  };
}
std::vector<X86Register> BinaryInstr::Defs() const {
  switch (kind) {
  case CMP:
  case TEST:
    return {};
  default:
    if (dst.IsReg()) {
      return {dst.GetReg()};
    } else {
      return {};
    }
  };
}
std::vector<X86Register> LabelInstr::Defs() const { return {}; }
std::vector<X86Register> CallInstr::Defs() const {
  std::vector<X86Register> defs(std::begin(CALLER_SAVE), std::end(CALLER_SAVE));
  defs.push_back(EAX);
  return defs;
}
std::vector<X86Register> JmpInstr::Defs() const { return {}; }
std::vector<X86Register> JInstr::Defs() const { return {}; }
std::vector<X86Register> RetInstr::Defs() const { return {}; }

std::vector<Label> UnaryInstr::Jumps() const { return {}; }
std::vector<Label> BinaryInstr::Jumps() const { return {}; }
std::vector<Label> LabelInstr::Jumps() const { return {}; }
std::vector<Label> CallInstr::Jumps() const { return {}; }
std::vector<Label> JmpInstr::Jumps() const { return {target}; }
std::vector<Label> JInstr::Jumps() const { return {target}; }
std::vector<Label> RetInstr::Jumps() const { return {}; }

bool UnaryInstr::IsFallThrough() const { return true; }
bool BinaryInstr::IsFallThrough() const { return true; }
bool LabelInstr::IsFallThrough() const { return true; }
bool CallInstr::IsFallThrough() const { return true; }
bool JmpInstr::IsFallThrough() const { return false; }
bool JInstr::IsFallThrough() const { return true; }
bool RetInstr::IsFallThrough() const { return true; }

std::optional<Label> UnaryInstr::IsLabel() const { return std::nullopt; }
std::optional<Label> BinaryInstr::IsLabel() const { return std::nullopt; }
std::optional<Label> LabelInstr::IsLabel() const { return label; }
std::optional<Label> CallInstr::IsLabel() const { return std::nullopt; }
std::optional<Label> JmpInstr::IsLabel() const { return std::nullopt; }
std::optional<Label> JInstr::IsLabel() const { return std::nullopt; }
std::optional<Label> RetInstr::IsLabel() const { return std::nullopt; }

std::optional<std::pair<X86Register, X86Register>>
UnaryInstr::IsMoveBetweenTemps() const {
  return std::nullopt;
}
std::optional<std::pair<X86Register, X86Register>>
BinaryInstr::IsMoveBetweenTemps() const {
  if (kind == MOV && src.IsReg() && dst.IsReg()) {
    return {{dst.GetReg(), src.GetReg()}};
  }
  // TODO: LEA
  return std::nullopt;
}
std::optional<std::pair<X86Register, X86Register>>
LabelInstr::IsMoveBetweenTemps() const {
  return std::nullopt;
}
std::optional<std::pair<X86Register, X86Register>>
CallInstr::IsMoveBetweenTemps() const {
  return std::nullopt;
}
std::optional<std::pair<X86Register, X86Register>>
JmpInstr::IsMoveBetweenTemps() const {
  return std::nullopt;
}
std::optional<std::pair<X86Register, X86Register>>
JInstr::IsMoveBetweenTemps() const {
  return std::nullopt;
}
std::optional<std::pair<X86Register, X86Register>>
RetInstr::IsMoveBetweenTemps() const {
  return std::nullopt;
}

void UnaryInstr::rename(std::function<X86Register(X86Register)> &sigma) {
  src.rename(sigma);
}
void BinaryInstr::rename(std::function<X86Register(X86Register)> &sigma) {
  src.rename(sigma);
  dst.rename(sigma);
}
void LabelInstr::rename(std::function<X86Register(X86Register)> &sigma) {}
void CallInstr::rename(std::function<X86Register(X86Register)> &sigma) {}
void JmpInstr::rename(std::function<X86Register(X86Register)> &sigma) {}
void JInstr::rename(std::function<X86Register(X86Register)> &sigma) {}
void RetInstr::rename(std::function<X86Register(X86Register)> &sigma) {}

void UnaryInstr::accept(X86InstrVisitor &visitor) { visitor.Visit(*this); }
void BinaryInstr::accept(X86InstrVisitor &visitor) { visitor.Visit(*this); }
void LabelInstr::accept(X86InstrVisitor &visitor) { visitor.Visit(*this); }
void CallInstr::accept(X86InstrVisitor &visitor) { visitor.Visit(*this); }
void JmpInstr::accept(X86InstrVisitor &visitor) { visitor.Visit(*this); }
void JInstr::accept(X86InstrVisitor &visitor) { visitor.Visit(*this); }
void RetInstr::accept(X86InstrVisitor &visitor) { visitor.Visit(*this); }

} // namespace mjc
