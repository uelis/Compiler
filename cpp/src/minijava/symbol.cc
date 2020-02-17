#include "minijava/symbol.h"

#include "minijava/error.h"

namespace mjc {

MethodSymbol::MethodSymbol(Ident method_name, std::vector<VarDecl> parameters,
                           std::vector<VarDecl> locals,
                           std::shared_ptr<Type> return_type,
                           bool throws_io_exception, bool is_static)
    : method_name_(method_name),
      return_type_(return_type),
      throws_io_exception_(throws_io_exception),
      is_static_(is_static) {
  for (auto &p : parameters) {
    if (parameters_.contains(p.var_name))
      throw CompileError("Duplicate parameter name.", p.source_location);
    parameters_.insert(p.var_name, p.var_type);
  }
  for (auto &l : locals) {
    if (parameters_.contains(l.var_name))
      throw CompileError("Local variable shadows parameter.",
                         l.source_location);
    if (locals_.contains(l.var_name))
      throw CompileError("Duplicate local.", l.source_location);
    locals_.insert(l.var_name, l.var_type);
  }
};

MethodSymbol::MethodSymbol(const MethodDecl &md)
    : MethodSymbol(md.method_name, md.parameters, md.locals, md.return_type,
                   md.throws_io_exception, false){};

MethodSymbol::MethodSymbol(const MainClassDecl &mcd)
    : MethodSymbol("main", {}, {}, std::make_shared<TypeVoid>(),
                   mcd.main_throws_io_exception, true){};

bool MethodSymbol::IsStatic() const { return is_static_; }

const Ident &MethodSymbol::GetName() const { return method_name_; }

const OrderedMap<Ident, std::shared_ptr<Type>> &MethodSymbol::GetParameters()
    const {
  return parameters_;
}

const OrderedMap<Ident, std::shared_ptr<Type>> &MethodSymbol::GetLocals()
    const {
  return locals_;
}

const std::shared_ptr<Type> MethodSymbol::GetReturnType() const {
  return return_type_;
}

bool MethodSymbol::ThrowsIOException() const { return throws_io_exception_; }

ClassSymbol::ClassSymbol(const ClassDecl &cd) : class_name_(cd.class_name) {
  for (auto &f : cd.fields) {
    if (fields_.contains(f.var_name)) {
      throw CompileError("Duplicate instance variable name.",
                         f.source_location);
    }
    fields_.insert(f.var_name, f.var_type);
  }

  for (const auto &md : cd.methods) {
    if (methods_.contains(md.method_name)) {
      throw CompileError("Duplicate method name.", md.source_location);
    }
    methods_.emplace(md.method_name, md);
  }
}

ClassSymbol::ClassSymbol(const MainClassDecl &mcd)
    : class_name_(mcd.class_name) {
  methods_.emplace("main", mcd);
}

const Ident &ClassSymbol::GetName() const { return class_name_; }

const OrderedMap<Ident, std::shared_ptr<Type>> &ClassSymbol::GetFields() const {
  return fields_;
}

const OrderedMap<Ident, MethodSymbol> &ClassSymbol::GetMethods() const {
  return methods_;
}

SymbolTable::SymbolTable(const Program &prg)
    : main_class_(prg.main_class.class_name) {
  classes_.emplace(prg.main_class.class_name, prg.main_class);
  for (auto &cd : prg.classes) {
    if (classes_.contains(cd.class_name)) {
      throw CompileError("Duplicate class name.", cd.source_location);
    }
    classes_.emplace(cd.class_name, cd);
  }
}

const Ident &SymbolTable::GetMainClass() const { return main_class_; }

const OrderedMap<Ident, ClassSymbol> &SymbolTable::GetClasses() const {
  return classes_;
}

}  // namespace mjc