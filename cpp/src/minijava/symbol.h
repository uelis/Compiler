//
// Symbol table
//

#ifndef MJC_MINIJAVA_SYMBOL_H
#define MJC_MINIJAVA_SYMBOL_H

#include "minijava/ast.h"
#include "util/ordered_map.h"

namespace mjc {

// Represents one entry for a method in the symbol table
class MethodSymbol {
 public:
  MethodSymbol(const MethodDecl &md);
  MethodSymbol(const MainClassDecl &md);
  MethodSymbol(const MethodSymbol &ms) = delete;

  bool IsStatic() const;
  const Ident &GetName() const;
  const OrderedMap<Ident, std::shared_ptr<Type>> &GetParameters() const;
  const OrderedMap<Ident, std::shared_ptr<Type>> &GetLocals() const;
  const std::shared_ptr<Type> GetReturnType() const;
  bool ThrowsIOException() const;

 private:
  MethodSymbol(Ident method_name, std::vector<VarDecl> parameters,
               std::vector<VarDecl> locals, std::shared_ptr<Type> return_type,
               bool throws_io_exception, bool is_static);

  Ident method_name_;
  OrderedMap<Ident, std::shared_ptr<Type>> parameters_;
  OrderedMap<Ident, std::shared_ptr<Type>> locals_;
  std::shared_ptr<Type> return_type_;
  bool throws_io_exception_;
  bool is_static_;
};

// Entry for a class in the symbol table
class ClassSymbol {
 public:
  ClassSymbol(const ClassDecl &cd);
  ClassSymbol(const MainClassDecl &mcd);
  ClassSymbol(const ClassSymbol &cs) = delete;

  const Ident &GetName() const;
  const OrderedMap<Ident, std::shared_ptr<Type>> &GetFields() const;
  const OrderedMap<Ident, MethodSymbol> &GetMethods() const;

 private:
  const Ident class_name_;
  OrderedMap<Ident, std::shared_ptr<Type>> fields_;
  OrderedMap<Ident, MethodSymbol> methods_;
};

// Represents the symbol table for a whole MiniJava program
class SymbolTable {
 public:
  SymbolTable(const Program &prg);
  SymbolTable(const SymbolTable &symbols) = delete;

  const Ident &GetMainClass() const;
  const OrderedMap<Ident, ClassSymbol> &GetClasses() const;

 private:
  const Ident main_class_;
  OrderedMap<Ident, ClassSymbol> classes_;
};

}  // namespace mjc

#endif
