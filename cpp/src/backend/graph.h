//
// Simple graph data structure
//
#ifndef MJC_BACKEND_GRAPH_H
#define MJC_BACKEND_GRAPH_H

#include <set>
#include <unordered_map>
#include <unordered_set>

namespace mjc {

template <typename T>
class Graph {
 public:
  Graph() : successors_{} {}
  Graph(const Graph &) = delete;
  Graph(Graph &&) = default;

  using node_set = std::unordered_set<T>;
  using successor_set = std::set<T>;
  using iterator =
      typename std::unordered_map<T, successor_set>::const_iterator;

  const successor_set &GetSuccessors(T n) const {
    auto it = successors_.find(n);
    return (it != successors_.end()) ? it->second : empty_;
  }
  void AddEdge(T s, T d) { successors_[s].insert(d); }
  size_t OutDegree(const T &n) { return successors_[n].size(); }
  iterator begin() const { return successors_.begin(); }
  iterator end() const { return successors_.end(); }
  size_t size() const { return successors_.size(); }

 private:
  std::unordered_map<T, successor_set> successors_;
  static successor_set const empty_;
};

template <typename T>
typename Graph<T>::successor_set const Graph<T>::empty_ = std::set<T>{};

}  // namespace mjc

#endif
