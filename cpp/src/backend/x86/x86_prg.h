//
// x86 machine programs
//
#ifndef MJC_BACKEND_X86PRG_H
#define MJC_BACKEND_X86PRG_H

#include <memory>
#include <iostream>

#include "backend/x86/x86_instr.h"
#include "backend/x86/x86_function.h"

namespace mjc {

// Represents an x86 machine program
struct X86Prg {
  std::vector<std::unique_ptr<X86Function>> functions;
};

std::ostream& operator<<(std::ostream &os, X86Prg &p);

}  // namespace mjc

#endif
