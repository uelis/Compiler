//
// Register allocation by graph colouring
//
#ifndef MJC_BACKEND_REGALLOC_H
#define MJC_BACKEND_REGALLOC_H

#include <algorithm>
#include <functional>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#include "backend/flow.h"
#include "backend/interference.h"
#include "backend/liveness.h"

namespace mjc {

template <typename Target>
class RegAlloc {
  using R = typename Target::Reg;
  using I = typename Target::Instr;
  using F = typename Target::Function;
  using P = typename Target::Prg;

 public:
  void Process(P &prg) {
    for (auto &f : prg.functions) {
      Regalloc(*f);
    }
  }

 private:
  class colour_result {
   public:
    colour_result(const colour_result &) = delete;
    colour_result(colour_result &&) = default;
    std::unordered_map<R, R> colouring;
    std::vector<R> spills;
  };

  void Regalloc(F &fun) {
    auto interference = Build(fun);
    auto stack = SimplifyAndSpill(interference);
    auto result = Select(interference, stack);
    if (result.spills.size() == 0) {
      std::function<R(R)> sigma = [&result](R t) {
        auto it = result.colouring.find(t);
        return (it == result.colouring.end()) ? Target::GENERAL_PURPOSE_REGS[0]
                                              : it->second;
      };
      fun.rename(sigma);
    } else {
      fun.spill(result.spills);
      Regalloc(fun);
    }
  }

  Interference<Target> Build(F &fun) {
    auto flow = FlowGraph<Target>(fun);
    auto liveness = Liveness<Target>(fun, flow);
    return Interference<Target>(fun, liveness);
  }

  std::stack<R> SimplifyAndSpill(const Interference<Target> &interference) {
    auto stack = std::stack<R>{};
    auto &graph = interference.GetGraph();
    auto K = Target::GENERAL_PURPOSE_REGS.size();
    auto low_degrees = std::vector<R>{};
    auto high_degrees = std::unordered_map<R, unsigned>{};

    for (auto &[t, t_succ] : std::as_const(graph)) {
      if (t.IsMachineReg()) continue;
      auto deg = t_succ.size();
      if (deg < K) {
        low_degrees.push_back(t);
      } else {
        high_degrees[t] = deg;
      }
    }

    while (low_degrees.size() + high_degrees.size() > 0) {
      R next_temp;
      if (low_degrees.size() > 0) {
        next_temp = low_degrees.back();
        low_degrees.pop_back();
      } else {
        auto max_degree = static_cast<unsigned>(0);
        for (auto &[t, deg] : high_degrees) {
          if (deg > max_degree) {
            next_temp = t;
            max_degree = deg;
          }
        }
        high_degrees.erase(next_temp);
      }

      stack.push(next_temp);
      for (auto &t : graph.GetSuccessors(next_temp)) {
        auto it = high_degrees.find(t);
        if (it != high_degrees.end()) {
          auto deg = --it->second;
          if (deg == K - 1) {
            high_degrees.erase(t);
            low_degrees.push_back(t);
          }
        }
      }
    }
    return stack;
  }

  colour_result Select(const Interference<Target> &interference,
                       std::stack<R> &stack) {
    auto result = colour_result{};
    auto &graph = interference.GetGraph();

    result.colouring.reserve(graph.size());
    for (const R &t : Target::MACHINE_REGS) {
      result.colouring[t] = t;
    }

    auto usable_colours = std::unordered_set<R>{};
    usable_colours.insert(Target::GENERAL_PURPOSE_REGS.begin(),
                          Target::GENERAL_PURPOSE_REGS.end());

    while (stack.size() > 0) {
      auto s = stack.top();
      stack.pop();

      auto possible_colours = usable_colours;
      for (const auto &t : graph.GetSuccessors(s)) {
        auto it = result.colouring.find(t);
        if (it != result.colouring.end()) {
          possible_colours.erase(it->second);
        }
      }
      if (possible_colours.begin() != possible_colours.end()) {
        result.colouring[s] = *possible_colours.begin();
      } else {
        result.spills.push_back(s);
      }
    }
    return result;
  }
};
}  // namespace mjc

#endif
