#include "backend/x86/x86_registers.h"

namespace mjc {

X86Register::X86Register() : number{} {}

X86Register::X86Register(unsigned number) : number(number) {}

X86Register::X86Register(const Temp &t) : number(t.GetId() + NUMBER_OF_REGS) {}

bool X86Register::IsMachineReg() const { return number < NUMBER_OF_REGS; }

} // namespace mjc

namespace std {
size_t hash<mjc::X86Register>::operator()(const mjc::X86Register &r) const {
  return std::hash<unsigned>{}(r.number);
}
} // namespace std
