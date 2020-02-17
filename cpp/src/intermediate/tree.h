//
// Abstract syntax of the tree intermediate language.
// Expressions and statements are represented by the composites in
// tree_exp.h and tree_stm.h.
//
#ifndef MJC_MINIJAVA_TREE_H
#define MJC_MINIJAVA_TREE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "intermediate/tree_exp.h"
#include "intermediate/tree_stm.h"

namespace mjc {

// Represents a single function in a tree program
struct TreeFunction {
  Label name;
  std::size_t parameter_count;
  std::vector<std::unique_ptr<TreeStm>> body; // must not contain nullptr
  Temp return_temp;
};

struct TreeProgram {
  std::vector<TreeFunction> functions;
};

std::ostream &operator<<(std::ostream &os, TreeFunction &fun);
std::ostream &operator<<(std::ostream &os, TreeProgram &prg);
}

#endif
