//
// Interface of x86 backend
//
#ifndef MJC_BACKEND_X86TARGET_H
#define MJC_BACKEND_X86TARGET_H

#include "backend/x86/x86_prg.h"
#include "intermediate/names.h"
#include "intermediate/tracer.h"

namespace mjc {

class X86Target {
public:
  using Reg = X86Register;
  using Instr = X86Instr;
  using Function = X86Function;
  using Prg = X86Prg;

  static const int WORD_SIZE = 4;
  static const std::vector<Reg> MACHINE_REGS;
  static const std::vector<Reg> GENERAL_PURPOSE_REGS;

  static Prg CodeGen(Tracer::TracedTreeProgram &prg);
};

} // namespace mjc
#endif
