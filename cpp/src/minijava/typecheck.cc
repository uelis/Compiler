#include "minijava/typecheck.h"

#include <cassert>
#include <optional>
#include <sstream>

#include "minijava/error.h"

namespace mjc {

// Visitor that checks if a type is well-formed. This includes checking
// that the type uses only classes that are defined somewhere in the program.
class CheckTypeWellFormed : public TypeVisitor<void> {
 public:
  CheckTypeWellFormed(const SymbolTable &symbols) : symbols_(symbols) {}
  virtual void VisitVoid(const TypeVoid t) {}
  virtual void VisitInt(const TypeInt t) {}
  virtual void VisitBool(const TypeBool t) {}
  virtual void VisitArray(const TypeArray t) { Visit(t.GetElem()); }
  virtual void VisitClass(const TypeClass t) {
    if (!symbols_.GetClasses().contains(t.GetName())) {
      std::stringstream err;
      err << "Use of defined class " << t.GetName();
      throw CompileError(err.str(), t.GetLocation());
    }
  };

 private:
  const SymbolTable &symbols_;
};

std::shared_ptr<Type> lookup_id(const ClassSymbol &class_symbol,
                                const MethodSymbol &method_symbol,
                                const Ident &id) {
  auto tl = method_symbol.GetLocals().find(id);
  if (tl != method_symbol.GetLocals().cend()) return tl->second;

  auto tp = method_symbol.GetParameters().find(id);
  if (tp != method_symbol.GetParameters().cend()) return tp->second;

  auto tf = class_symbol.GetFields().find(id);
  if (tf != class_symbol.GetFields().cend()) return tf->second;

  return {};
}

// Type inference for expressions
class TypeOfExp : public ExpVisitor<std::shared_ptr<Type>> {
 public:
  TypeOfExp(const SymbolTable &symbols, const ClassSymbol &class_symbol,
            const MethodSymbol &method_symbol)
      : symbols_(symbols),
        class_symbol_(class_symbol),
        method_symbol_(method_symbol) {}

  void expect(const Exp &e, const Type &expected) {
    auto actual = Visit(e);
    if (*actual != expected) {
      std::stringstream err;
      err << "Expression has type " << *actual << ", but type " << expected
          << " is expected.";
      throw CompileError(err.str(), e.GetLocation());
    }
  }

  virtual std::shared_ptr<Type> VisitNum(const ExpNum &e) {
    return std::make_shared<TypeInt>();
  };

  virtual std::shared_ptr<Type> VisitId(const ExpId &e) {
    auto const &id = e.GetId();
    auto r = lookup_id(class_symbol_, method_symbol_, id);
    if (!r) {
      throw CompileError("Undeclared variable ", e.GetLocation());
    }
    return r;
  };

  virtual std::shared_ptr<Type> VisitBinOp(const ExpBinOp &e) {
    switch (e.GetBinOp()) {
      case ExpBinOp::BinOp::PLUS:
      case ExpBinOp::BinOp::MINUS:
      case ExpBinOp::BinOp::MUL:
      case ExpBinOp::BinOp::DIV: {
        expect(e.GetLeft(), TypeInt());
        expect(e.GetRight(), TypeInt());
        return std::make_shared<TypeInt>();
      }
      case ExpBinOp::BinOp::LT: {
        expect(e.GetLeft(), TypeInt());
        expect(e.GetRight(), TypeInt());
        return std::make_shared<TypeBool>();
      }
      case ExpBinOp::BinOp::STRICTAND: {
        expect(e.GetLeft(), TypeBool());
        expect(e.GetRight(), TypeBool());
        return std::make_shared<TypeBool>();
      }
    }
    assert(false);
    return {};
  }

  virtual std::shared_ptr<Type> VisitInvoke(const ExpInvoke &e) {
    std::shared_ptr<Type> tobj = Visit(e.GetObj());

    if (tobj->GetOp() != Type::TypeClassOp) {
      std::stringstream err;
      err << "Expression has type " << tobj << ", but class type is required.";
      throw CompileError(err.str(), e.GetObj().GetLocation());
    }

    auto const &cls = static_cast<TypeClass &>(*tobj).GetName();
    auto const cdi = symbols_.GetClasses().find(cls);
    assert(cdi != symbols_.GetClasses()
                      .cend());  // type checker returns only well-formed types.
    auto const &cd = cdi->second;

    auto mdi = cd.GetMethods().find(e.GetMethod());
    if (mdi == cd.GetMethods().cend()) {
      std::stringstream err;
      err << "Call of undefined method " << cls << "." << e.GetMethod() << ".";
      throw CompileError(err.str(), e.GetLocation());
    }
    const auto &md = mdi->second;

    // Check argument types
    auto argi = e.GetArgs().begin();
    for (auto &pi : md.GetParameters().keys()) {
      if (argi == e.GetArgs().end()) {
        throw CompileError("Method call has too few arguments.",
                           e.GetLocation());
      }
      expect(**argi, *md.GetParameters().find(pi)->second);
      argi++;
    }
    if (argi != e.GetArgs().end()) {
      throw CompileError("Method call has too many arguments.",
                         e.GetLocation());
    }

    return md.GetReturnType();
  }

  virtual std::shared_ptr<Type> VisitArrayGet(const ExpArrayGet &e) {
    expect(e.GetArray(), TypeArray{std::make_shared<TypeInt>()});
    expect(e.GetIndex(), TypeInt{});
    return std::make_shared<TypeInt>();
  }

  virtual std::shared_ptr<Type> VisitArrayLength(const ExpArrayLength &e) {
    expect(e.GetArray(), TypeArray{std::make_shared<TypeInt>()});
    return std::make_shared<TypeInt>();
  }

  virtual std::shared_ptr<Type> VisitTrue(const ExpTrue &e) {
    return std::make_shared<TypeBool>();
  }

  virtual std::shared_ptr<Type> VisitFalse(const ExpFalse &e) {
    return std::make_shared<TypeBool>();
  }

  virtual std::shared_ptr<Type> VisitThis(const ExpThis &e) {
    if (method_symbol_.IsStatic()) {
      std::stringstream err;
      err << "Cannot use `this` in static context.";
      throw CompileError(err.str(), e.GetLocation());
    }
    return std::make_shared<TypeClass>(class_symbol_.GetName());
  }

  virtual std::shared_ptr<Type> VisitNew(const ExpNew &e) {
    if (!symbols_.GetClasses().contains(e.GetCls())) {
      std::stringstream err;
      err << "Undefined class " << e.GetCls() << ".";
      throw CompileError(err.str(), e.GetLocation());
    }
    return std::make_shared<TypeClass>(e.GetCls());
  }

  virtual std::shared_ptr<Type> VisitNewIntArray(const ExpNewIntArray &e) {
    expect(e.GetSize(), TypeInt{});
    return std::make_shared<TypeArray>(std::make_shared<TypeInt>());
  }

  virtual std::shared_ptr<Type> VisitNeg(const ExpNeg &e) {
    expect(e.GetExp(), TypeBool{});
    return std::make_shared<TypeBool>();
  }

  virtual std::shared_ptr<Type> VisitRead(const ExpRead &e) {
    return std::make_shared<TypeInt>();
  }

 private:
  const SymbolTable &symbols_;
  const ClassSymbol &class_symbol_;
  const MethodSymbol &method_symbol_;
};

// Type checking for statements
class StmOk : public StmVisitor<void> {
 public:
  StmOk(const SymbolTable &symbols, const ClassSymbol &class_symbol,
        const MethodSymbol &method_symbol)
      : method_symbol_(method_symbol),
        class_symbol_(class_symbol),
        type_of_exp_(symbols, class_symbol, method_symbol) {}

  void VisitAssignment(const StmAssignment &s) {
    auto tid = lookup_id(class_symbol_, method_symbol_, s.GetId());
    if (!tid) {
      std::stringstream err;
      err << "Undeclared variable + " + s.GetId() << ".";
      throw CompileError(err.str(), s.GetLocation());
    }
    auto texp = type_of_exp_.Visit(s.GetExp());
    if (*tid != *texp) {
      std::stringstream err;
      err << "Assignment of expression of type " << *texp
          << " to variable of type " << *tid;
      throw CompileError(err.str(), s.GetLocation());
    }
  };

  void VisitArrayAssignment(const StmArrayAssignment &s) {
    auto tid = lookup_id(class_symbol_, method_symbol_, s.GetId());
    if (!tid) throw CompileError("Undeclared variable + " + s.GetId());
    if (tid->GetOp() != Type::TypeArrayOp)
      throw CompileError("Variable " + s.GetId() + " must have array type.");
    const Type &telem(static_cast<TypeArray &>(*tid).GetElem());

    type_of_exp_.expect(s.GetIndex(), TypeInt{});
    type_of_exp_.expect(s.GetExp(), telem);
  };

  void VisitIf(const StmIf &s) {
    type_of_exp_.expect(s.GetCond(), TypeBool{});
    Visit(s.GetTrueBranch());
    Visit(s.GetFalseBranch());
  }

  void VisitWhile(const StmWhile &s) {
    type_of_exp_.expect(s.GetCond(), TypeBool{});
    Visit(s.GetBody());
  }

  void VisitPrint(const StmPrint &s) {
    type_of_exp_.expect(s.GetExp(), TypeInt{});
  };

  void VisitWrite(const StmWrite &s) {
    type_of_exp_.expect(s.GetExp(), TypeInt{});
  };

  void VisitSeq(const StmSeq &s) {
    for (auto &s1 : s.GetStms()) {
      Visit(*s1);
    }
  }

 private:
  const MethodSymbol &method_symbol_;
  const ClassSymbol &class_symbol_;
  TypeOfExp type_of_exp_;
};

class ExpThrows : public ExpVisitor<bool> {
 public:
  ExpThrows(const SymbolTable &symbols, const ClassSymbol &class_symbol,
            const MethodSymbol &method_symbol)
      : symbols_(symbols),
        class_symbol_(class_symbol),
        method_symbol_(method_symbol) {}

  virtual bool VisitNum(const ExpNum &e) { return false; }

  virtual bool VisitId(const ExpId &e) { return false; }

  virtual bool VisitBinOp(const ExpBinOp &e) {
    return Visit(e.GetLeft()) || Visit(e.GetRight());
  }

  virtual bool VisitInvoke(const ExpInvoke &e) {
    bool throws = Visit(e.GetObj());

    for (auto &arg : e.GetArgs()) {
      throws |= Visit(*arg);
    }

    auto tobj =
        TypeOfExp{symbols_, class_symbol_, method_symbol_}.Visit(e.GetObj());
    auto const &cls = static_cast<TypeClass &>(*tobj).GetName();
    auto cd = symbols_.GetClasses().find(cls);
    assert(cd != symbols_.GetClasses().cend());
    auto md = cd->second.GetMethods().find(e.GetMethod());
    assert(md != cd->second.GetMethods().cend());
    throws |= md->second.ThrowsIOException();

    return throws;
  }

  virtual bool VisitArrayGet(const ExpArrayGet &e) {
    return Visit(e.GetArray()) || Visit(e.GetIndex());
  }

  virtual bool VisitArrayLength(const ExpArrayLength &e) {
    return Visit(e.GetArray());
  }

  virtual bool VisitTrue(const ExpTrue &e) { return false; }

  virtual bool VisitFalse(const ExpFalse &e) { return false; }

  virtual bool VisitThis(const ExpThis &e) { return false; }

  virtual bool VisitNew(const ExpNew &e) { return false; }

  virtual bool VisitNewIntArray(const ExpNewIntArray &e) {
    return Visit(e.GetSize());
  }

  virtual bool VisitNeg(const ExpNeg &e) { return Visit(e.GetExp()); }

  virtual bool VisitRead(const ExpRead &e) { return true; }

 private:
  const SymbolTable &symbols_;
  const ClassSymbol &class_symbol_;
  const MethodSymbol &method_symbol_;
};

class StmThrows : public StmVisitor<bool> {
 public:
  StmThrows(const SymbolTable &symbols, const ClassSymbol &class_symbol,
            const MethodSymbol &method_symbol)
      : exp_throws_(symbols, class_symbol, method_symbol) {}

  bool VisitAssignment(const StmAssignment &s) {
    return exp_throws_.Visit(s.GetExp());
  };

  bool VisitArrayAssignment(const StmArrayAssignment &s) {
    return exp_throws_.Visit(s.GetIndex()) || exp_throws_.Visit(s.GetExp());
  };

  bool VisitIf(const StmIf &s) {
    return exp_throws_.Visit(s.GetCond()) || Visit(s.GetTrueBranch()) ||
           Visit(s.GetFalseBranch());
  }

  bool VisitWhile(const StmWhile &s) {
    return exp_throws_.Visit(s.GetCond()) || Visit(s.GetBody());
  }

  bool VisitPrint(const StmPrint &s) { return exp_throws_.Visit(s.GetExp()); }

  bool VisitWrite(const StmWrite &s) { return exp_throws_.Visit(s.GetExp()); }

  bool VisitSeq(const StmSeq &s) {
    bool throws = false;
    for (auto &s1 : s.GetStms()) {
      throws |= Visit(*s1);
    }
    return throws;
  }

 private:
  ExpThrows exp_throws_;
};

void typecheck(const SymbolTable &symbols, const ClassSymbol &cs,
               const MethodDecl &md) {
  auto msi = cs.GetMethods().find(md.method_name);
  assert(msi != cs.GetMethods().cend());
  auto &ms = msi->second;

  for (auto &p : md.parameters) {
    CheckTypeWellFormed(symbols).Visit(*p.var_type);
  }

  for (auto &l : md.locals) {
    CheckTypeWellFormed(symbols).Visit(*l.var_type);
  }

  StmOk(symbols, cs, ms).Visit(*md.body);
  auto return_type = TypeOfExp(symbols, cs, ms).Visit(*md.return_exp);

  if (*return_type != *md.return_type) {
    std::stringstream err;
    err << "The return expression has type " << *return_type
        << ", but the method's return type is " << *md.return_type << ".";
    throw CompileError(err.str(), md.return_exp->GetLocation());
  }

  bool throws = StmThrows(symbols, cs, ms).Visit(*md.body) ||
                ExpThrows(symbols, cs, ms).Visit(*md.return_exp);
  if (throws && !ms.ThrowsIOException()) {
    std::stringstream err;
    err << "Method body may throw IOException, which is not declared.";
    throw CompileError(err.str(), md.source_location);
  }
}

void typecheck(const SymbolTable &symbols, const ClassDecl &cd) {
  auto csi = symbols.GetClasses().find(cd.class_name);
  assert(csi != symbols.GetClasses().cend());
  auto &cs = csi->second;

  for (auto &f : cd.fields) {
    CheckTypeWellFormed(symbols).Visit(*f.var_type);
  }

  for (auto &md : cd.methods) {
    typecheck(symbols, cs, md);
  }
}

void typecheck(const SymbolTable &symbols, const MainClassDecl &md) {
  auto csi = symbols.GetClasses().find(md.class_name);
  assert(csi != symbols.GetClasses().cend());
  auto &cs = csi->second;
  auto msi = cs.GetMethods().find("main");
  assert(msi != cs.GetMethods().cend());
  auto &ms = msi->second;

  StmOk(symbols, cs, ms).Visit(*md.main_body);

  if (StmThrows(symbols, cs, ms).Visit(*md.main_body) &&
      !ms.ThrowsIOException()) {
    std::stringstream err;
    err << "Method body may throw IOException, which is not declared.";
    throw CompileError(err.str(), md.source_location);
  }
}

void Typecheck(const SymbolTable &symbols, const Program &prg) {
  typecheck(symbols, prg.main_class);
  for (auto &cd : prg.classes) {
    typecheck(symbols, cd);
  }
}

std::shared_ptr<Type> TypeOf(const SymbolTable &symbols,
                             const ClassSymbol &class_symbol,
                             const MethodSymbol &method_symbol,
                             const Exp &exp) {
  return TypeOfExp(symbols, class_symbol, method_symbol).Visit(exp);
}
}  // namespace mjc
