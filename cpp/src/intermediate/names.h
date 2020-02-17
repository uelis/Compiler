#ifndef MJC_INTERMEDIATE_NAMES_H
#define MJC_INTERMEDIATE_NAMES_H

#include <iostream>
#include <variant>

namespace mjc {
class Temp;
class Label;
} // namespace mjc

namespace std {

template <> struct hash<mjc::Temp> {
  size_t operator()(const mjc::Temp &f) const;
};

template <> struct hash<mjc::Label> {
  size_t operator()(const mjc::Label &f) const;
};

} // namespace std

namespace mjc {

// Generates thread-local fresh names
class Temp {
public:
  // Generates a fresh (thread-locally) temp of the form "ti"
  Temp();

  // Makes a temp with a fixed value.
  // The user must avoid name clashes.
  Temp(unsigned fixed);

  int GetId() const;

  bool operator==(const Temp &other) const;

private:
  int id_;
};

std::ostream &operator<<(std::ostream &out, const Temp &temp);

// Generates fresh labels
class Label {
public:
  // Generates a fresh label of the form "L$i"
  Label();

  // Label with a fixed name
  Label(std::string fixed);

  bool operator==(const Label &other) const;

private:
  std::variant<Temp, std::string> label_;

  friend size_t std::hash<Label>::operator()(const Label &) const;
  friend std::ostream &operator<<(std::ostream &out, const Label &label);
};

} // namespace mjc

#endif
