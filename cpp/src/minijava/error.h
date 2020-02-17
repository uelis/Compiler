//
// Simple error representation and reporting
//

#ifndef MJC_MINIJAVA_ERROR_H
#define MJC_MINIJAVA_ERROR_H

#include <optional>
#include <string>

#include "minijava/location.hh"

namespace mjc {

// Represents a compile error, such as type errors, and has methods for
// error reporting.
class CompileError {
public:
  CompileError(std::string msg) : msg_(msg), location_(std::nullopt) {}
  CompileError(std::string msg, std::optional<location> l)
      : msg_(msg), location_(l) {}

  const std::string &GetMessage() const;

  // Prints error report to stderr
  void report(const std::string file) const;

private:
  std::string msg_;
  std::optional<location> location_;

  void report_context(const std::string file) const;
};
} // namespace mjc
#endif
