#ifndef MJC_INTERMEDIATE_MINIJAVA_TO_TREE_H
#define MJC_INTERMEDIATE_MINIJAVA_TO_TREE_H

#include "intermediate/tree.h"
#include "minijava/ast.h"
#include "minijava/symbol.h"
#include "minijava/typecheck.h"

namespace mjc {

// Translation from MiniJava to the tree intermediate language
//
// Translation of types:
// - int -> int32
// - boolean -> int32: 0 = false, 1 = true
// - int[] -> int32: address of a memory block containing
//                   length, a[0], ..., a[length-1]
//                   (all int32 values)
// - Object -> int32: address of a memort block containing
//                    class-id, field1, field2, ..., fieldn
//                    (all int32 values)
//
// Translation of methods:
// - A f(B x, C y) in class D becomes
//   int32 D$f(int32 _this, int32 x, int32 x)
//
// Translation of classes:
// - gather all translated methods
template <typename TargetMachine>
class MinijavaToTree {
 public:
  MinijavaToTree(const SymbolTable &symbols) : symbols_(symbols) {}

  TreeProgram Process(const Program &prg) { return Translate(prg); }

 private:
  const SymbolTable &symbols_;

  using upTreeExp = std::unique_ptr<TreeExp>;
  using upTreeStm = std::unique_ptr<TreeStm>;

  // current position in translation
  const ClassSymbol *class_symbol_;
  const MethodSymbol *method_symbol_;
  std::unordered_map<std::string, Temp> local_temps_;

  ///////////////////////////////////////////////////////////////////
  // Program
  ///////////////////////////////////////////////////////////////////

  TreeProgram Translate(const Program &prg) {
    std::vector<TreeFunction> functions;
    // classes
    for (auto &cd : prg.classes) {
      class_symbol_ = &symbols_.GetClasses().find(cd.class_name)->second;
      for (auto &md : cd.methods) {
        functions.push_back(Translate(md));
      }
    }
    // main class
    functions.push_back(Translate(prg.main_class));
    return {.functions = std::move(functions)};
  }

  TreeFunction Translate(const MainClassDecl &mcd) {
    class_symbol_ = &symbols_.GetClasses().find(mcd.class_name)->second;
    auto it = class_symbol_->GetMethods().find("main");
    assert(it != class_symbol_->GetMethods().cend());
    method_symbol_ = &it->second;

    local_temps_.clear();

    Temp ret;
    std::vector<upTreeStm> body;
    body.push_back(std::move(TranslateStm(*this).Visit(*mcd.main_body)));
    body.push_back(std::make_unique<TreeStmMove>(
        std::make_unique<TreeExpTemp>(ret), std::make_unique<TreeExpConst>(0)));
    AppendRaiseBlock(body);

    return {.name = Label("Lmain"),
            .parameter_count = 1,
            .body = std::move(body),
            .return_temp = ret};
  }

  ///////////////////////////////////////////////////////////////////
  // Methods
  ///////////////////////////////////////////////////////////////////

  TreeFunction Translate(const MethodDecl &md) {
    assert(class_symbol_);
    method_symbol_ = &class_symbol_->GetMethods().find(md.method_name)->second;

    local_temps_.clear();
    for (auto &p : md.locals) {
      local_temps_[p.var_name] = Temp{};
    }

    Temp ret;
    std::vector<upTreeStm> body;
    body.push_back(TranslateStm(*this).Visit(*md.body));
    body.push_back(std::make_unique<TreeStmMove>(
        std::make_unique<TreeExpTemp>(ret),
        TranslateExp(*this).Visit(*md.return_exp)));
    AppendRaiseBlock(body);

    return {.name = Runtime::FunctionName(class_symbol_->GetName(),
                                          method_symbol_->GetName()),
            .parameter_count = 1 + md.parameters.size(),
            .body = std::move(body),
            .return_temp = ret};
  }

  ///////////////////////////////////////////////////////////////////
  // Statements
  ///////////////////////////////////////////////////////////////////

  class TranslateStm : public StmVisitor<upTreeStm> {
   public:
    explicit TranslateStm(const MinijavaToTree &outer) : outer_(outer){};

    virtual upTreeStm VisitAssignment(const StmAssignment &s) {
      auto x = outer_.VarLExp(s.GetId());
      auto e = TranslateExp(outer_).Visit(s.GetExp());
      return std::make_unique<TreeStmMove>(std::move(x), std::move(e));
    }

    virtual upTreeStm VisitArrayAssignment(const StmArrayAssignment &s) {
      auto translate_exp = TranslateExp(outer_);
      auto raise = Runtime::RaiseBlockName(outer_.class_symbol_->GetName(),
                                           outer_.method_symbol_->GetName());
      auto d = Runtime::ArrayDeref(outer_.VarLExp(s.GetId()),
                                   translate_exp.Visit(s.GetIndex()), raise);
      auto &stms = d.first;
      auto &exp = d.second;
      stms.push_back(std::make_unique<TreeStmMove>(
          std::move(exp), translate_exp.Visit(s.GetExp())));
      return std::make_unique<TreeStmSeq>(std::move(stms));
    }

    virtual upTreeStm VisitIf(const StmIf &s) {
      auto l_true = Label{};
      auto l_false = Label{};
      auto l_end = Label{};
      auto stms = std::vector<upTreeStm>{};
      stms.push_back(TranslateCond(outer_, l_true, l_false).Visit(s.GetCond()));
      stms.push_back(std::make_unique<TreeStmLabel>(l_true));
      stms.push_back(Visit(s.GetTrueBranch()));
      stms.push_back(std::make_unique<TreeStmJump>(l_end));
      stms.push_back(std::make_unique<TreeStmLabel>(l_false));
      stms.push_back(Visit(s.GetFalseBranch()));
      stms.push_back(std::make_unique<TreeStmLabel>(l_end));
      return std::make_unique<TreeStmSeq>(std::move(stms));
    }

    virtual upTreeStm VisitWhile(const StmWhile &s) {
      auto l_loop = Label{};
      auto l_true = Label{};
      auto l_end = Label{};
      auto stms = std::vector<upTreeStm>{};
      stms.push_back(std::make_unique<TreeStmLabel>(l_loop));
      stms.push_back(TranslateCond(outer_, l_true, l_end).Visit(s.GetCond()));
      stms.push_back(std::make_unique<TreeStmLabel>(l_true));
      stms.push_back(Visit(s.GetBody()));
      stms.push_back(std::make_unique<TreeStmJump>(l_loop));
      stms.push_back(std::make_unique<TreeStmLabel>(l_end));
      return std::make_unique<TreeStmSeq>(std::move(stms));
    }

    virtual upTreeStm VisitPrint(const StmPrint &s) {
      return std::make_unique<TreeStmMove>(
          std::make_unique<TreeExpTemp>(Temp{}),
          std::make_unique<TreeExpCall>(
              Runtime::PrintFunction(),
              TranslateExp(outer_).Visit(s.GetExp())));
    }

    virtual upTreeStm VisitWrite(const StmWrite &s) {
      return std::make_unique<TreeStmMove>(
          std::make_unique<TreeExpTemp>(Temp{}),
          std::make_unique<TreeExpCall>(
              Runtime::WriteFunction(),
              TranslateExp(outer_).Visit(s.GetExp())));
    }

    virtual upTreeStm VisitSeq(const StmSeq &s) {
      auto ts = std::vector<upTreeStm>{};
      for (auto &si : s.GetStms()) {
        ts.push_back(Visit(*si));
      }
      return std::make_unique<TreeStmSeq>(std::move(ts));
    }

   private:
    const MinijavaToTree &outer_;
  };

  ///////////////////////////////////////////////////////////////////
  // Expressions
  ///////////////////////////////////////////////////////////////////

  class TranslateExp : public ExpVisitor<upTreeExp> {
   public:
    explicit TranslateExp(const MinijavaToTree &outer) : outer_(outer){};

    virtual upTreeExp VisitNum(const ExpNum &e) {
      return std::make_unique<TreeExpConst>(e.GetNum());
    };

    virtual upTreeExp VisitId(const ExpId &e) {
      return outer_.VarLExp(e.GetId());
    }

    virtual upTreeExp VisitBinOp(const ExpBinOp &e) {
      std::optional<TreeExpBinOp::BinOp> op;
      switch (e.GetBinOp()) {
        case ExpBinOp::BinOp::PLUS:
          op = TreeExpBinOp::BinOp::PLUS;
          break;
        case ExpBinOp::BinOp::MINUS:
          op = TreeExpBinOp::BinOp::MINUS;
          break;
        case ExpBinOp::BinOp::MUL:
          op = TreeExpBinOp::BinOp::MUL;
          break;
        case ExpBinOp::BinOp::DIV:
          op = TreeExpBinOp::BinOp::DIV;
          break;
        default:
          // boolean operation
          auto t = Temp{};
          auto l_true = Label{};
          auto l_false = Label{};
          auto stms = std::vector<upTreeStm>{};
          stms.push_back(
              std::make_unique<TreeStmMove>(std::make_unique<TreeExpTemp>(t),
                                            std::make_unique<TreeExpConst>(0)));
          stms.push_back(TranslateCond(outer_, l_true, l_false).Visit(e));
          stms.push_back(std::make_unique<TreeStmLabel>(l_true));
          stms.push_back(
              std::make_unique<TreeStmMove>(std::make_unique<TreeExpTemp>(t),
                                            std::make_unique<TreeExpConst>(1)));
          stms.push_back(std::make_unique<TreeStmLabel>(l_false));
          return std::make_unique<TreeExpESeq>(
              std::move(stms), std::make_unique<TreeExpTemp>(t));
      }
      assert(op);
      return std::make_unique<TreeExpBinOp>(*op, Visit(e.GetLeft()),
                                            Visit(e.GetRight()));
    };

    virtual upTreeExp VisitInvoke(const ExpInvoke &e) {
      std::shared_ptr<Type> ty = TypeOf(outer_.symbols_, *outer_.class_symbol_,
                                        *outer_.method_symbol_, e.GetObj());
      assert(ty->GetOp() == Type::TypeClassOp);
      auto tyclass = static_cast<TypeClass &>(*ty);
      auto cls = tyclass.GetName();

      auto args = std::vector<upTreeExp>{};
      args.push_back(Visit(e.GetObj()));
      for (auto &a : e.GetArgs()) {
        args.push_back(Visit(*a));
      }
      return std::make_unique<TreeExpCall>(
          std::make_unique<TreeExpName>(
              Runtime::FunctionName(cls, e.GetMethod())),
          std::move(args));
    }

    virtual upTreeExp VisitArrayGet(const ExpArrayGet &e) {
      auto raise = Runtime::RaiseBlockName(outer_.class_symbol_->GetName(),
                                           outer_.method_symbol_->GetName());
      auto d =
          Runtime::ArrayDeref(Visit(e.GetArray()), Visit(e.GetIndex()), raise);
      return std::make_unique<TreeExpESeq>(std::move(d.first),
                                           std::move(d.second));
    }

    virtual upTreeExp VisitArrayLength(const ExpArrayLength &e) {
      return Runtime::ArrayLength(Visit(e.GetArray()));
    }

    virtual upTreeExp VisitTrue(const ExpTrue &e) {
      return std::make_unique<TreeExpConst>(1);
    }

    virtual upTreeExp VisitFalse(const ExpFalse &e) {
      return std::make_unique<TreeExpConst>(0);
    }

    virtual upTreeExp VisitThis(const ExpThis &e) {
      return Runtime::ThisAddress();
    }

    virtual upTreeExp VisitNew(const ExpNew &e) {
      return Runtime::NewObject(outer_.symbols_, e.GetCls());
    }

    virtual upTreeExp VisitNewIntArray(const ExpNewIntArray &e) {
      return Runtime::NewIntArray(Visit(e.GetSize()));
    }

    virtual upTreeExp VisitNeg(const ExpNeg &e) {
      return std::make_unique<TreeExpBinOp>(TreeExpBinOp::BinOp::MINUS,
                                            std::make_unique<TreeExpConst>(1),
                                            Visit(e.GetExp()));
    }

    virtual upTreeExp VisitRead(const ExpRead &e) {
      return std::make_unique<TreeExpCall>(
          std::make_unique<TreeExpName>(Runtime::ReadFunction()),
          std::vector<upTreeExp>{});
    }

   private:
    const MinijavaToTree &outer_;
  };

  ///////////////////////////////////////////////////////////////////
  // Conditions
  ///////////////////////////////////////////////////////////////////

  class TranslateCond : public ExpVisitor<upTreeStm> {
   public:
    explicit TranslateCond(const MinijavaToTree &outer, const Label &l_true,
                           const Label &l_false)
        : outer_(outer), l_true_(l_true), l_false_(l_false){};

    upTreeStm generic(const Exp &e) const {
      auto t = Temp{};
      auto stms = std::vector<upTreeStm>{};
      stms.push_back(std::make_unique<TreeStmMove>(
          std::make_unique<TreeExpTemp>(t), TranslateExp(outer_).Visit(e)));
      stms.push_back(std::make_unique<TreeStmCJump>(
          TreeStmCJump::RelOp::EQ, std::make_unique<TreeExpTemp>(t),
          std::make_unique<TreeExpConst>(1), l_true_, l_false_));
      return std::make_unique<TreeStmSeq>(std::move(stms));
    }

    virtual upTreeStm VisitNum(const ExpNum &e) {
      assert(false);
      abort();
    };

    virtual upTreeStm VisitId(const ExpId &e) { return generic(e); }

    virtual upTreeStm VisitBinOp(const ExpBinOp &e) {
      //      std::optional<TreeExpBinOp::BinOp> op;
      switch (e.GetBinOp()) {
        case ExpBinOp::BinOp::STRICTAND: {
          auto l = Label{};
          auto stms = std::vector<upTreeStm>{};
          stms.push_back(TranslateCond(outer_, l, l_false_).Visit(e.GetLeft()));
          stms.push_back(std::make_unique<TreeStmLabel>(l));
          stms.push_back(
              TranslateCond(outer_, l_true_, l_false_).Visit(e.GetRight()));
          return std::make_unique<TreeStmSeq>(std::move(stms));
        }
        case ExpBinOp::BinOp::LT: {
          auto l = Temp{};
          auto r = Temp{};
          auto stms = std::vector<upTreeStm>{};
          stms.push_back(std::make_unique<TreeStmMove>(
              std::make_unique<TreeExpTemp>(l),
              TranslateExp(outer_).Visit(e.GetLeft())));
          stms.push_back(std::make_unique<TreeStmMove>(
              std::make_unique<TreeExpTemp>(r),
              TranslateExp(outer_).Visit(e.GetRight())));
          stms.push_back(std::make_unique<TreeStmCJump>(
              TreeStmCJump::RelOp::LT, std::make_unique<TreeExpTemp>(l),
              std::make_unique<TreeExpTemp>(r), l_true_, l_false_));
          return std::make_unique<TreeStmSeq>(std::move(stms));
        }
        default:
          assert(false);
          abort();
      }
    };

    virtual upTreeStm VisitInvoke(const ExpInvoke &e) { return generic(e); }

    virtual upTreeStm VisitArrayGet(const ExpArrayGet &e) {
      assert(false);
      abort();
    }

    virtual upTreeStm VisitArrayLength(const ExpArrayLength &e) {
      assert(false);
      abort();
    }

    virtual upTreeStm VisitTrue(const ExpTrue &e) {
      return std::make_unique<TreeStmJump>(l_true_);
    }

    virtual upTreeStm VisitFalse(const ExpFalse &e) {
      return std::make_unique<TreeStmJump>(l_false_);
    }

    virtual upTreeStm VisitThis(const ExpThis &e) {
      assert(false);
      abort();
    }

    virtual upTreeStm VisitNew(const ExpNew &e) {
      assert(false);
      abort();
    }

    virtual upTreeStm VisitNewIntArray(const ExpNewIntArray &e) {
      assert(false);
      abort();
    }

    virtual upTreeStm VisitNeg(const ExpNeg &e) {
      return TranslateCond(outer_, l_false_, l_true_).Visit(e.GetExp());
    }

    virtual upTreeStm VisitRead(const ExpRead &e) {
      assert(false);
      abort();
    }

   private:
    const MinijavaToTree &outer_;
    const Label l_true_;
    const Label l_false_;
  };

  ///////////////////////////////////////////////////////////////////
  // Helpers
  ///////////////////////////////////////////////////////////////////

  upTreeExp VarLExp(const std::string &id) const {
    assert(class_symbol_);
    assert(method_symbol_);
    auto t = local_temps_.find(id);
    if (t != local_temps_.end()) {
      return std::make_unique<TreeExpTemp>(t->second);
    } else {
      const auto &params = method_symbol_->GetParameters().keys();
      const auto pi = std::find(params.cbegin(), params.cend(), id);
      if (pi != params.end()) {
        int n = std::distance(params.begin(), pi) + 1 /* 0 is 'this' */;
        return std::make_unique<TreeExpParam>(n);
      } else {
        const auto &fields = class_symbol_->GetFields().keys();
        const auto fi = std::find(fields.begin(), fields.end(), id);
        if (fi != fields.end()) {
          int n = std::distance(fields.begin(), fi);
          auto this_addr = Runtime::ThisAddress();
          auto field_addr = Runtime::FieldAddress(std::move(this_addr), n);
          return std::make_unique<TreeExpMem>(std::move(field_addr));
        } else {
          assert(false);  // type-correctness
          return nullptr;
        }
      }
    }
  }

  void AppendRaiseBlock(std::vector<upTreeStm> &stms) {
    assert(class_symbol_);
    assert(method_symbol_);

    auto end = Label{};
    auto raise = Runtime::RaiseBlockName(class_symbol_->GetName(),
                                         method_symbol_->GetName());

    stms.push_back(std::make_unique<TreeStmJump>(end));
    stms.push_back(std::make_unique<TreeStmLabel>(raise));
    stms.push_back(std::make_unique<TreeStmMove>(
        std::make_unique<TreeExpTemp>(Temp{}),
        std::make_unique<TreeExpCall>(Runtime::RaiseFunction(),
                                      std::make_unique<TreeExpConst>(1))));
    stms.push_back(std::make_unique<TreeStmJump>(raise));
    stms.push_back(std::make_unique<TreeStmLabel>(end));
  }

  ///////////////////////////////////////////////////////////////////
  // Runtime
  ///////////////////////////////////////////////////////////////////

  struct Runtime {
    static Label FunctionName(const std::string &class_name,
                              const std::string &method_name) {
      return Label{"L" + class_name + "$" + method_name};
    }

    static Label RaiseBlockName(const std::string &class_name,
                                const std::string &method_name) {
      return {"L" + class_name + "$" + method_name + "$raise"};
    }

    static upTreeExp FieldAddress(upTreeExp obj, int n) {
      return std::make_unique<TreeExpBinOp>(
          TreeExpBinOp::BinOp::PLUS, std::move(obj),
          std::make_unique<TreeExpConst>((n + 1) * TargetMachine::WORD_SIZE));
    }

    static upTreeExp ThisAddress() { return std::make_unique<TreeExpParam>(0); }

    static Label ReadFunction() { return {"L_read"}; }

    static Label WriteFunction() { return {"L_write"}; }

    static Label PrintFunction() { return {"L_println_int"}; }

    static Label RaiseFunction() { return {"L_raise"}; }

    static upTreeExp ArrayAddr(upTreeExp ea, upTreeExp ei) {
      auto len1 = std::make_unique<TreeExpBinOp>(
          TreeExpBinOp::BinOp::PLUS, std::move(ei),
          std::make_unique<TreeExpConst>(1));
      auto offset = std::make_unique<TreeExpBinOp>(
          TreeExpBinOp::BinOp::MUL, std::move(len1),
          std::make_unique<TreeExpConst>(
              static_cast<int32_t>(TargetMachine::WORD_SIZE)));
      auto addr = std::make_unique<TreeExpBinOp>(
          TreeExpBinOp::BinOp::PLUS, std::move(ea), std::move(offset));
      return addr;
    }

    static upTreeExp ArrayLength(upTreeExp ea) {
      return std::make_unique<TreeExpMem>(std::move(ea));
    }

    static std::pair<std::vector<upTreeStm>, upTreeExp> ArrayDeref(
        upTreeExp ea, upTreeExp ei, Label &l_raise) {
      if (ei->GetOp() == TreeExp::TreeExpConstOp) {
        int32_t c = static_cast<TreeExpConst &>(*ei).GetValue();
        if (c < 0) {
          auto stms = std::vector<upTreeStm>{};
          stms.push_back(std::make_unique<TreeStmJump>(l_raise));
          return {std::move(stms), std::make_unique<TreeExpTemp>(Temp())};
        } else {
          auto ta = Temp{};
          auto l_ok = Label{};
          auto stms = std::vector<upTreeStm>{};
          stms.push_back(std::make_unique<TreeStmMove>(
              std::make_unique<TreeExpTemp>(ta), std::move(ea)));
          stms.push_back(std::make_unique<TreeStmCJump>(
              TreeStmCJump::RelOp::LT, std::make_unique<TreeExpConst>(c),
              ArrayLength(std::make_unique<TreeExpTemp>(ta)), l_ok, l_raise));
          stms.push_back(std::make_unique<TreeStmLabel>(l_ok));
          auto exp = std::make_unique<TreeExpMem>(
              ArrayAddr(std::make_unique<TreeExpTemp>(ta),
                        std::make_unique<TreeExpConst>(c)));
          return {std::move(stms), std::move(exp)};
        }
      } else {
        auto ta = Temp{};
        auto ti = Temp{};
        auto l_check_upper = Label{};
        auto l_ok = Label{};
        auto stms = std::vector<upTreeStm>{};
        stms.push_back(std::make_unique<TreeStmMove>(
            std::make_unique<TreeExpTemp>(ta), std::move(ea)));
        stms.push_back(std::make_unique<TreeStmMove>(
            std::make_unique<TreeExpTemp>(ti), std::move(ei)));
        stms.push_back(std::make_unique<TreeStmCJump>(
            TreeStmCJump::RelOp::GE, std::make_unique<TreeExpTemp>(ti),
            std::make_unique<TreeExpConst>(0), l_check_upper, l_raise));
        stms.push_back(std::make_unique<TreeStmLabel>(l_check_upper));
        stms.push_back(std::make_unique<TreeStmCJump>(
            TreeStmCJump::RelOp::LT, std::make_unique<TreeExpTemp>(ti),
            ArrayLength(std::make_unique<TreeExpTemp>(ta)), l_ok, l_raise));
        stms.push_back(std::make_unique<TreeStmLabel>(l_ok));
        auto exp = std::make_unique<TreeExpMem>(
            ArrayAddr(std::make_unique<TreeExpTemp>(ta),
                      std::make_unique<TreeExpTemp>(ti)));
        return {std::move(stms), std::move(exp)};
      }
    }

    static upTreeExp NewObject(const SymbolTable &symbols,
                               const std::string &cls) {
      auto alloc = Label{"L_halloc"};
      auto clsit = symbols.GetClasses().find(cls);
      auto size = 1 + clsit->second.GetFields().keys().size();
      return std::make_unique<TreeExpCall>(
          alloc,
          std::make_unique<TreeExpConst>(size * TargetMachine::WORD_SIZE));
    }

    static upTreeExp NewIntArray(upTreeExp len) {
      auto alloc = Label{"L_halloc"};
      auto tlen = Temp{};
      auto taddr = Temp{};
      auto stms = std::vector<upTreeStm>{};
      stms.push_back(std::make_unique<TreeStmMove>(
          std::make_unique<TreeExpTemp>(tlen), std::move(len)));
      stms.push_back(std::make_unique<TreeStmMove>(
          std::make_unique<TreeExpTemp>(taddr),
          std::make_unique<TreeExpCall>(
              alloc, std::make_unique<TreeExpBinOp>(
                         TreeExpBinOp::BinOp::MUL,
                         std::make_unique<TreeExpConst>(
                             static_cast<int32_t>(TargetMachine::WORD_SIZE)),
                         std::make_unique<TreeExpBinOp>(
                             TreeExpBinOp::BinOp::PLUS,
                             std::make_unique<TreeExpTemp>(tlen),
                             std::make_unique<TreeExpConst>(1))))));
      stms.push_back(std::make_unique<TreeStmMove>(
          std::make_unique<TreeExpMem>(std::make_unique<TreeExpTemp>(taddr)),
          std::make_unique<TreeExpTemp>(tlen)));
      return std::make_unique<TreeExpESeq>(
          std::move(stms), std::make_unique<TreeExpTemp>(taddr));
    }
  };
};
}  // namespace mjc
#endif
