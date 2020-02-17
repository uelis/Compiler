#include "minijava/error.h"

#include <fstream>

namespace mjc {

void CompileError::report(const std::string file) const {
  report_context(file);
  std::cerr << std::endl;
  std::cerr << "Error: " << msg_ << std::endl;
}

void CompileError::report_context(const std::string file) const {
  if (!location_) return;
  auto begin = location_->begin;
  auto end = location_->end;

  std::cerr << file << ":" << begin.line << ":" << begin.column << ":" << std::endl;
  std::cerr << std::endl;

  if (std::ifstream f{file}) {
    const int context = 1;
    unsigned int line_no = 1;
    std::string line;
    while (std::getline(f, line) && line_no <= end.line + context) {
      if (begin.line <= line_no + context) {
        std::cerr << '|' << line << std::endl;
        if (line_no >= begin.line && line_no <= end.line) {
          std::cerr << '|';
          for (std::size_t col_no = 0; col_no < line.size(); col_no++) {
            bool outside = false;
            outside |= line_no == begin.line && col_no + 1 < begin.column;
            outside |= line_no == end.line && col_no + 1 >= end.column;
            char c = line[col_no] == '\t' ? '\t' : (outside ? ' ' : '^');
            std::cerr << c;
          }
          std::cerr << std::endl;
        }
      }
      line_no++;
    }
  }
}

const std::string &CompileError::GetMessage() const { return msg_; }

}
