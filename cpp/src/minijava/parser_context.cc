#include "minijava/parser_context.h"

#include "minijava/error.h"
#include "minijava/parser.hh"

namespace mjc {
Program &ParserContext::Parse() {
  scan_begin();
  parser{*this}.parse();  // sets result_
  scan_end();
  if (error_) {
    throw error_;
  }
  return result_;
}

void ParserContext::SetParseError(const location &l, const std::string &m) {
  throw CompileError(m, l);
}

void ParserContext::SetParseError(const std::string &m) {
  throw CompileError(m);
}

void ParserContext::SetResult(Program prg) { result_ = std::move(prg); }

void ParserContext::scan_begin() {
  if (file_.empty() || file_ == "-") {
    yyin = stdin;
  } else if (!(yyin = fopen(file_.c_str(), "r"))) {
    throw CompileError("File not found: " + file_);
  }
}

void ParserContext::scan_end() {
  fclose(yyin);
  yylex_destroy();
}

}  // namespace mjc
