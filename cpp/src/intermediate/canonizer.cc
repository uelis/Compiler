#include "intermediate/canonizer.h"

namespace mjc {
using upTreeExp = std::unique_ptr<TreeExp>;
using upTreeStm = std::unique_ptr<TreeStm>;

class ESeq;
TreeProgram Canonize(TreeProgram prg);
TreeFunction Canonize(TreeFunction fun);
std::vector<upTreeStm> Canonize(std::vector<upTreeStm> stms);
std::vector<upTreeStm> Canonize(upTreeStm stm);
ESeq CanonizeNoTopCall(upTreeExp exp);
ESeq Canonize(upTreeExp exp);

class ESeq {
 public:
  std::vector<upTreeStm> stms_;
  upTreeExp exp_;

  ESeq();
  ESeq(upTreeExp exp);
  ESeq(std::vector<upTreeStm> stms, upTreeExp exp);

  static bool Commutes(const TreeStm &stm, const TreeExp &exp);

  static bool Commute(const std::vector<upTreeStm> &stms, const TreeExp &exp);

  void Extend(std::vector<upTreeStm> stms);

  void CombineWith(ESeq b, std::function<upTreeExp(upTreeExp, upTreeExp)> f);

  void CombineWith(
      std::vector<ESeq> bs,
      std::function<upTreeExp(upTreeExp, std::vector<upTreeExp>)> f);

  static std::vector<upTreeStm> CombineToStm(
      ESeq b1, ESeq b2, std::function<upTreeStm(upTreeExp, upTreeExp)> f);
};

ESeq::ESeq(){};

ESeq::ESeq(upTreeExp exp) : exp_(std::move(exp)){};

ESeq::ESeq(std::vector<upTreeStm> stms, upTreeExp exp)
    : stms_(std::move(stms)), exp_(std::move(exp)){};

bool ESeq::Commutes(const TreeStm &stm, const TreeExp &exp) {
  return exp.GetOp() == TreeExp::TreeExpNameOp ||
         exp.GetOp() == TreeExp::TreeExpConstOp;
}

bool ESeq::Commute(const std::vector<upTreeStm> &stms, const TreeExp &exp) {
  for (const auto &stm : stms) {
    if (!Commutes(*stm, exp)) {
      return false;
    }
  }
  return true;
}

void ESeq::Extend(std::vector<upTreeStm> stms) {
  if (Commute(stms, *exp_)) {
    std::move(stms.begin(), stms.end(), std::inserter(stms_, stms_.end()));
  } else {
    Temp i;
    stms_.push_back(std::make_unique<TreeStmMove>(
        std::make_unique<TreeExpTemp>(i), std::move(exp_)));
    std::move(stms.begin(), stms.end(), std::inserter(stms_, stms_.end()));
    exp_ = std::make_unique<TreeExpTemp>(i);
  }
}

void ESeq::CombineWith(ESeq b,
                       std::function<upTreeExp(upTreeExp, upTreeExp)> f) {
  Extend(std::move(b.stms_));
  exp_ = f(std::move(exp_), std::move(b.exp_));
}

void ESeq::CombineWith(
    std::vector<ESeq> bs,
    std::function<upTreeExp(upTreeExp, std::vector<upTreeExp>)> f) {
  auto es = std::vector<upTreeExp>{};
  auto ss = std::vector<upTreeStm>{};
  for (auto &b : bs) {
    b.Extend(std::move(ss));
    ss = std::move(b.stms_);
    es.push_back(std::move(b.exp_));
  }
  Extend(std::move(ss));
  exp_ = f(std::move(exp_), std::move(es));
}

std::vector<upTreeStm> ESeq::CombineToStm(
    ESeq b1, ESeq b2, std::function<upTreeStm(upTreeExp, upTreeExp)> f) {
  b1.Extend(std::move(b2.stms_));
  b1.stms_.push_back(f(std::move(b1.exp_), std::move(b2.exp_)));
  return std::move(b1.stms_);
}

TreeProgram Canonize(TreeProgram prg) {
  auto functions = std::vector<TreeFunction>{};
  for (auto &fun : prg.functions) {
    functions.push_back(Canonize(std::move(fun)));
  };
  return TreeProgram{.functions = std::move(functions)};
}

TreeFunction Canonize(TreeFunction fun) {
  return {.name = fun.name,
          .parameter_count = fun.parameter_count,
          .body = Canonize(std::move(fun.body)),
          .return_temp = fun.return_temp};
}

std::vector<upTreeStm> Canonize(std::vector<upTreeStm> stms) {
  std::vector<upTreeStm> res;
  for (auto &stm : stms) {
    auto stms = Canonize(std::move(stm));
    std::move(stms.begin(), stms.end(), std::inserter(res, res.end()));
  }
  return res;
}

std::vector<upTreeStm> Canonize(upTreeStm stm) {
  class Stm : public TreeStmVisitor<std::vector<upTreeStm>> {
    virtual std::vector<upTreeStm> VisitMove(TreeStmMove &s) {
      class MoveCase : public TreeExpVisitor<std::vector<upTreeStm>> {
       public:
        MoveCase(upTreeExp dst, upTreeExp src)
            : dst_(std::move(dst)), src_(std::move(src)){};

        std::vector<upTreeStm> match() { return this->Visit(*dst_); }

       private:
        upTreeExp dst_;
        upTreeExp src_;

        virtual std::vector<upTreeStm> VisitConst(TreeExpConst &e) {
          assert(false);
          return {};
        }

        virtual std::vector<upTreeStm> VisitName(TreeExpName &e) {
          assert(false);
          return {};
        }

        virtual std::vector<upTreeStm> VisitTemp(TreeExpTemp &e) {
          auto b = Canonize(std::move(src_));
          b.stms_.push_back(std::make_unique<TreeStmMove>(std::move(dst_),
                                                          std::move(b.exp_)));
          return std::move(b.stms_);
        }

        virtual std::vector<upTreeStm> VisitParam(TreeExpParam &e) {
          auto b = Canonize(std::move(src_));
          b.stms_.push_back(std::make_unique<TreeStmMove>(std::move(dst_),
                                                          std::move(b.exp_)));
          return std::move(b.stms_);
        }

        virtual std::vector<upTreeStm> VisitMem(TreeExpMem &e) {
          auto b1 = CanonizeNoTopCall(std::move(e.GetAddr()));
          auto b2 = Canonize(std::move(src_));
          return ESeq::CombineToStm(
              std::move(b1), std::move(b2), [](auto e1, auto e2) {
                return std::make_unique<TreeStmMove>(
                    std::make_unique<TreeExpMem>(std::move(e1)), std::move(e2));
              });
        }

        virtual std::vector<upTreeStm> VisitBinOp(TreeExpBinOp &e) {
          assert(false);
          return {};
        }

        virtual std::vector<upTreeStm> VisitCall(TreeExpCall &e) {
          assert(false);
          return {};
        }

        virtual std::vector<upTreeStm> VisitESeq(TreeExpESeq &e) {
          auto stms = std::move(e.GetStms());
          stms.push_back(std::make_unique<TreeStmMove>(std::move(e.GetExp()),
                                                       std::move(src_)));
          return Canonize(std::move(stms));
        }
      };
      return MoveCase(std::move(s.GetDst()), std::move(s.GetSrc())).match();
    }

    virtual std::vector<upTreeStm> VisitJump(TreeStmJump &s) {
      auto b = CanonizeNoTopCall(std::move(s.GetTarget()));
      b.stms_.push_back(
          std::make_unique<TreeStmJump>(std::move(b.exp_), s.GetTargets()));
      return std::move(b.stms_);
    }

    virtual std::vector<upTreeStm> VisitCJump(TreeStmCJump &s) {
      auto b1 = CanonizeNoTopCall(std::move(s.GetLeft()));
      auto b2 = CanonizeNoTopCall(std::move(s.GetRight()));
      return ESeq::CombineToStm(
          std::move(b1), std::move(b2), [&s](auto e1, auto e2) {
            return std::make_unique<TreeStmCJump>(s.GetRel(), std::move(e1),
                                                  std::move(e2), s.GetLTrue(),
                                                  s.GetLFalse());
          });
    }

    virtual std::vector<upTreeStm> VisitLabel(TreeStmLabel &s) {
      std::vector<upTreeStm> stms;
      stms.push_back(std::make_unique<TreeStmLabel>(s.GetLabel()));
      return stms;
    }

    virtual std::vector<upTreeStm> VisitSeq(TreeStmSeq &s) {
      std::vector<upTreeStm> res;
      for (auto &stm : s.GetTreeStms()) {
        auto cstms = Visit(*stm);
        std::move(cstms.begin(), cstms.end(), std::inserter(res, res.end()));
      }
      return res;
    }
  };
  return Stm().Visit(*stm);
}

ESeq CanonizeNoTopCall(upTreeExp exp) {
  auto b = Canonize(std::move(exp));
  if (b.exp_->GetOp() == TreeExp::TreeExpCallOp) {
    Temp i;
    b.stms_.push_back(std::make_unique<TreeStmMove>(
        std::make_unique<TreeExpTemp>(i), std::move(b.exp_)));
    b.exp_ = std::make_unique<TreeExpTemp>(i);
  }
  return b;
}

ESeq Canonize(upTreeExp exp) {
  class Exp : public TreeExpVisitor<ESeq> {
   public:
    Exp(upTreeExp exp) : exp_(std::move(exp)) {}

    ESeq match() { return this->Visit(*exp_); }

   private:
    upTreeExp exp_;

    virtual ESeq VisitConst(TreeExpConst &e) { return ESeq(std::move(exp_)); }
    virtual ESeq VisitName(TreeExpName &e) { return ESeq(std::move(exp_)); }
    virtual ESeq VisitTemp(TreeExpTemp &e) { return ESeq(std::move(exp_)); }
    virtual ESeq VisitParam(TreeExpParam &e) { return ESeq(std::move(exp_)); }
    virtual ESeq VisitMem(TreeExpMem &e) {
      auto b = CanonizeNoTopCall(std::move(e.GetAddr()));
      b.exp_ = std::make_unique<TreeExpMem>(std::move(b.exp_));
      return b;
    }

    virtual ESeq VisitBinOp(TreeExpBinOp &e) {
      auto o = e.GetBinOp();
      auto b1 = CanonizeNoTopCall(std::move(e.GetLeft()));
      auto b2 = CanonizeNoTopCall(std::move(e.GetRight()));
      b1.CombineWith(std::move(b2), [o](auto e1, auto e2) {
        return std::make_unique<TreeExpBinOp>(o, std::move(e1), std::move(e2));
      });
      return b1;
    }

    virtual ESeq VisitCall(TreeExpCall &e) {
      auto b1 = CanonizeNoTopCall(std::move(e.GetFun()));
      std::vector<ESeq> bs;
      for (auto &arg : e.GetArgs()) {
        bs.push_back(CanonizeNoTopCall(std::move(arg)));
      }
      b1.CombineWith(std::move(bs), [](auto e, auto es) {
        return std::make_unique<TreeExpCall>(std::move(e), std::move(es));
      });
      return b1;
    }
    virtual ESeq VisitESeq(TreeExpESeq &e) {
      std::vector<upTreeStm> ss;
      for (auto &stm : e.GetStms()) {
        auto cstms = Canonize(std::move(stm));
        std::move(cstms.begin(), cstms.end(), std::inserter(ss, ss.end()));
      }
      return ESeq(std::move(ss), std::move(e.GetExp()));
    }
  };

  return Exp(std::move(exp)).match();
}

Canonizer::CanonizedTreeProgram Canonizer::Process(TreeProgram prg) {
  return CanonizedTreeProgram{Canonize(std::move(prg))};
}

}  // namespace mjc
