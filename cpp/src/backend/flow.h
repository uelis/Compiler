//
// Control flow graph for machine functions
//
#ifndef MJC_BACKEND_FLOW_H
#define MJC_BACKEND_FLOW_H

#include <unordered_map>

#include "backend/graph.h"

namespace mjc {

template <typename Target>
class FlowGraph {
  using R = typename Target::Reg;
  using I = typename Target::Instr;
  using F = typename Target::Function;

 public:
  FlowGraph(F& function) : graph_{} {
    auto const& body = function.GetBody();
    auto n = body.size();
    auto targets = std::unordered_map<Label, unsigned>{};

    for (unsigned i = 0; i < n; i++) {
      if (body[i]->IsLabel()) {
        targets[*body[i]->IsLabel()] = i;
      }
    }

    for (unsigned i = 0; i < n; i++) {
      if (i + 1 < n && body[i]->IsFallThrough()) {
        graph_.AddEdge(i, i + 1);
      }
      for (auto const& t : body[i]->Jumps()) {
        graph_.AddEdge(i, targets.find(t)->second);
      }
    }
  }

  const Graph<unsigned>& GetGraph() const { return graph_; }

 private:
  Graph<unsigned> graph_;
};

}  // namespace mjc

#endif
