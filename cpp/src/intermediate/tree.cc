#include "intermediate/tree.h"

#include <iostream>


namespace mjc {

std::ostream &operator<<(std::ostream &os, TreeFunction &fun) {
  os << fun.name << "(" << fun.parameter_count << ") {" << std::endl;
  for (auto &s : fun.body) {
    os << "  " << *s << std::endl;
  }
  os << "  return " << fun.return_temp << std::endl;
  os << "}" << std::endl;
  return os;
}

std::ostream &operator<<(std::ostream &os, TreeProgram &prg) {
  for (auto &f : prg.functions) {
    os << f << std::endl;
  }
  return os;
}
}