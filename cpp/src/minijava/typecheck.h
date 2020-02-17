//
// Type checking
//

#ifndef MJC_MINIJAVA_TYPECHECK_H
#define MJC_MINIJAVA_TYPECHECK_H

#include "minijava/ast.h"
#include "minijava/symbol.h"

namespace mjc {

// Type-checks a MiniJava program.
// Errors are reported by raising the exception CompileError.
void Typecheck(const SymbolTable &symbols, const Program &prg);

// Computes the type of an expression within a particular method
std::shared_ptr<Type> TypeOf(const SymbolTable &symbols,
                              const ClassSymbol &class_symbol,
                              const MethodSymbol &method_symbol,
                              const Exp &exp);

}  // namespace mjc

#endif
