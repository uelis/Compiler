//
// Interference graph for register allocation
//
#ifndef MJC_BACKEND_INTERFERENCE_H
#define MJC_BACKEND_INTERFERENCE_H

#include <set>
#include <unordered_set>
#include <vector>

#include "backend/graph.h"
#include "backend/liveness.h"

namespace mjc {

template <typename Target>
class Interference {
  using R = typename Target::Reg;
  using I = typename Target::Instr;
  using F = typename Target::Function;

 public:
  Interference(const Interference &i) = delete;
  Interference(Interference &&i) = default;

  Interference(F &function, const Liveness<Target> &liveness) {
    auto ignore = std::unordered_set<R>(Target::MACHINE_REGS.begin(),
                                        Target::MACHINE_REGS.end());
    for (auto r : Target::GENERAL_PURPOSE_REGS) {
      ignore.erase(r);
    }

    auto const &body = function.GetBody();
    int n = body.size();

    for (int i = 0; i < n; i++) {
      for (auto b : body[i]->Defs()) {
        if (ignore.find(b) != ignore.end()) continue;

        for (auto c : liveness.GetLiveOut(i)) {
          if (b == c) continue;
          if (ignore.find(c) != ignore.end()) continue;

          auto m = body[i]->IsMoveBetweenTemps();
          if (m && m->second == c) continue;
          interference_.AddEdge(b, c);
          interference_.AddEdge(c, b);
        }
      }
    }
  }

  const Graph<R> &GetGraph() const { return interference_; }

 private:
  Graph<R> interference_;
};

}  // namespace mjc

#endif
