//
// Liveness information for register allocation
//
#ifndef MJC_BACKEND_LIVENESS_H
#define MJC_BACKEND_LIVENESS_H

#include <unordered_set>
#include <vector>

#include "backend/graph.h"

namespace mjc {

template <typename Target>
class Liveness {
  using R = typename Target::Reg;
  using I = typename Target::Instr;
  using F = typename Target::Function;

 public:
  Liveness(const Liveness &) = delete;
  Liveness(Liveness &&) = default;

  Liveness(F &function, const FlowGraph<Target> &flow) {
    auto const &body = function.GetBody();
    auto n = body.size();
    live_in_.resize(n);
    live_out_.resize(n);

    bool change = true;
    while (change) {
      change = false;

      for (int a = n - 1; a >= 0; --a) {
        auto k = live_in_[a].size();
        for (auto &m : flow.GetGraph().GetSuccessors(a)) {
          for (auto &t : live_in_[m]) {
            if (live_out_[a].insert(t).second) {
              change = true;
              live_in_[a].insert(t);
            }
          }
        }

        for (auto t : body[a]->Defs()) {
          live_in_[a].erase(t);
        }
        for (auto t : body[a]->Uses()) {
          live_in_[a].insert(t);
        }
        change |= live_in_[a].size() > k;
      }
    }
  }

  const std::set<R> &GetLiveIn(int line) const { return live_in_[line]; }
  const std::set<R> &GetLiveOut(int line) const { return live_out_[line]; }

 private:
  std::vector<std::set<R>> live_in_;
  std::vector<std::set<R>> live_out_;
};

}  // namespace mjc

#endif
