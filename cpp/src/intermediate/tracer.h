//
// Tracing
//

#ifndef MJC_INTERMEDIATE_TRACER_H
#define MJC_INTERMEDIATE_TRACER_H

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "intermediate/canonizer.h"
#include "intermediate/names.h"
#include "intermediate/tree.h"

namespace mjc {

// Traces tree programs to establish the following invariant:
// - Any CJUMP(rel, e1, e2, l, r) is followed by LABEL(r), i.e. the false
//   branch.
// Tracing rearranges the program in this way while minimising unnecessary
// jumps.
class Tracer {
public:
  Tracer() {}

  // Empty class that marks a tree program as being traced.
  class TracedTreeProgram : public TreeProgram {
  private:
    TracedTreeProgram(TreeProgram &&prg) : TreeProgram(std::move(prg)) {}
    friend class Tracer;
  };

  static TracedTreeProgram
  Process(Canonizer::CanonizedTreeProgram prg);
};

} // namespace mjc

#endif
