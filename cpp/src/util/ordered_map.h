#ifndef UTIL_ORDERED_MAP_H
#define UTIL_ORDERED_MAP_H

#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <list>

namespace mjc {

template <typename key, typename value> class OrderedMap {
public:
  using const_iterator = typename std::unordered_map<key, value>::const_iterator;

  OrderedMap() : keys_(), values_(){};

  OrderedMap(std::initializer_list<std::pair<key, value>> l) {
    for (auto &p : l) {
      insert(p);
    }
  };

  void insert(const key k, value v) {
    if (contains(k)) {
      throw std::invalid_argument("duplicate insertion into ordered map");
    }
    keys_.push_back(k);
    values_[std::move(k)] = std::move(v);
  }

  void insert(std::pair<key, value> p) { insert(p.first, p.second); }

  template <class... Args> void emplace(const key k, Args &&... args) {
    if (contains(k)) {
      throw std::invalid_argument("duplicate insertion into ordered map");
    }
    keys_.push_back(k);
    values_.emplace(std::move(k), args...);
  }

  const_iterator cbegin() const {
    return values_.cbegin();
  }

  const_iterator cend() const {
    return values_.cend();
  }

  const_iterator find(const key &k) const {
    return values_.find(k);
  }

  bool contains(const key k) const { return values_.find(k) != values_.end(); }

  const std::list<key> &keys() const { return keys_; }

private:
  std::list<key> keys_;
  std::unordered_map<key, value> values_;
};
}

#endif
