#ifndef MJC_BACKEND_X86_REGISTERS_H
#define MJC_BACKEND_X86_REGISTERS_H

#include "intermediate/names.h"

namespace mjc {

class X86Register {
 public:
  unsigned number;

  X86Register();
  X86Register(unsigned number);
  X86Register(const Temp &temp);

  bool IsMachineReg() const;
  bool operator==(const X86Register &r) const { return number == r.number; }
  bool operator<(const X86Register &r) const { return number < r.number; }
};

static const int NUMBER_OF_REGS = 8;
static const X86Register EAX = 0;
static const X86Register EBX = 1;
static const X86Register ECX = 2;
static const X86Register EDX = 3;
static const X86Register ESI = 4;
static const X86Register EDI = 5;
static const X86Register EBP = 6;
static const X86Register ESP = 7;

static const X86Register CALLER_SAVE[] = {EAX, ECX, EDX};
static const X86Register CALLEE_SAVE[] = {EBX, ESI, EDI};

static const char *const REG_NAMES[] = {"eax", "ebx", "ecx", "edx",
                                        "esi", "edi", "ebp", "esp"};

}  // namespace mjc

namespace std {
template <>
struct hash<mjc::X86Register> {
  size_t operator()(const mjc::X86Register &r) const;
};
}  // namespace std
#endif
