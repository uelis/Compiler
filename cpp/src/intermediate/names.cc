#include "intermediate/names.h"

#include <string>
#include <iostream>

namespace mjc {

thread_local unsigned next_id_ = 0;

Temp::Temp() { id_ = next_id_++; }

Temp::Temp(unsigned fixed) { id_ = fixed; }

int Temp::GetId() const { return id_; }

bool Temp::operator==(const Temp &other) const { return id_ == other.id_; }

std::ostream &operator<<(std::ostream &out, const Temp &temp) {
  out << 't' << temp.GetId();
  return out;
}

Label::Label() {}

Label::Label(std::string fixed) : label_(std::move(fixed)) {}

bool Label::operator==(const Label &other) const {
  return label_ == other.label_;
}

std::ostream &operator<<(std::ostream &out, const Label &label) {
  if (std::holds_alternative<Temp>(label.label_)) {
    out << 'L' << std::get<Temp>(label.label_);
  } else {
    out << std::get<std::string>(label.label_);
  }
  return out;
}

} // namespace mjc

namespace std {

size_t hash<mjc::Temp>::operator()(const mjc::Temp &f) const {
  return std::hash<int>{}(f.GetId());
}

size_t hash<mjc::Label>::operator()(const mjc::Label &f) const {
  return std::hash<std::variant<mjc::Temp, std::string>>{}(f.label_);
}
} // namespace std
