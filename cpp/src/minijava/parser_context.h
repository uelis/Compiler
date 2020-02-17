#ifndef MINIJAVA_ParserContext_H
#define MINIJAVA_ParserContext_H

#include <map>
#include <string>

#include "minijava/ast.h"
#include "minijava/error.h"
#include "minijava/parser.hh"

#define YY_DECL                                                                \
  mjc::parser::symbol_type yylex(mjc::ParserContext &parser_context)
YY_DECL;
int yylex_destroy();
extern FILE *yyin;
extern mjc::location yyloc;

namespace mjc {

class ParserContext {
public:
  ParserContext(const std::string file) : file_(file) {}

  Program &Parse();

  void SetParseError(const mjc::location &l, const std::string &m);
  void SetParseError(const std::string &m);
  void SetResult(Program result);

private:
  std::string file_;
  Program result_;
  std::optional<CompileError> error_;

  void scan_begin();
  void scan_end();
};
} // namespace mjc
#endif
