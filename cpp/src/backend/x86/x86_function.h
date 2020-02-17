//
// x86 machine function
//
#ifndef MJC_BACKEND_x86FUNCTION_H
#define MJC_BACKEND_x86FUNCTION_H

#include <functional>
#include <memory>
#include <vector>

#include "backend/x86/x86_registers.h"
#include "intermediate/names.h"

namespace mjc {

class Operand;
class X86Instr;

class X86Function {
 public:
  X86Function(Label name, std::vector<std::unique_ptr<X86Instr>> body);

  virtual void rename(std::function<X86Register(X86Register)>& sigma);

  virtual void spill(std::vector<X86Register>& toSpill);

  const Label& GetName() const;
  const std::vector<std::unique_ptr<X86Instr>>& GetBody() const;
  unsigned GetFrameSize() const;

  virtual ~X86Function(){};

 private:
  Label name_;
  std::vector<std::unique_ptr<X86Instr>> body_;  // inv: contains no nullptr
  unsigned frame_size_ = 0;

  Operand AddLocalOnStack();
};

std::ostream& operator<<(std::ostream& os, X86Function& f);

}  // namespace mjc

#endif
