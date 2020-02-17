//
// Canonization of tree programs
//
#ifndef MJC_INTERMEDIATE_CANONIZER_H
#define MJC_INTERMEDIATE_CANONIZER_H

#include "intermediate/tree.h"
#include <functional>

namespace mjc {

// Canonizes tree programs
//
// Canonization brings the body of each function into a normal form with the
// following properties:
// - There is no ESEQ anymore.
// - Evaluation order is forced by the structure of programs, i.e.
//   sub-expressions can be evaluated in any order.
// - CALL can only appear in statements of the form MOVE(lexp, CALL(f, arg),
//   in particular, there can be no call in arg.
class Canonizer {
public:
  Canonizer() {}

  // Empty class that marks a tree program as being canonized.
  class CanonizedTreeProgram : public TreeProgram {
  private:
    CanonizedTreeProgram(TreeProgram &&prg) : TreeProgram(std::move(prg)) {}
    friend class Canonizer;
  };

  static CanonizedTreeProgram Process(TreeProgram prg);
};
} // namespace mjc
#endif
