#include "backend/x86/x86_target.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "backend/x86/x86_function.h"
#include "backend/x86/x86_instr.h"
#include "backend/x86/x86_prg.h"
#include "backend/x86/x86_registers.h"
#include "intermediate/tracer.h"
#include "intermediate/tree.h"
#include "intermediate/tree_exp.h"
#include "intermediate/tree_stm.h"

namespace mjc {
const std::vector<X86Register> X86Target::MACHINE_REGS{EAX, EBX, ECX, EDX,
                                                       ESI, EDI, ESP, EBP};

const std::vector<X86Register> X86Target::GENERAL_PURPOSE_REGS{EAX, EBX, ECX,
                                                               EDX, ESI, EDI};

class LinearCombination {
public:
  LinearCombination() {}
  LinearCombination(std::int32_t imm) : constant_(imm) {}
  LinearCombination(Temp temp) : coefficients_{{temp, 1}} {}

  unsigned NumberOfSummands() {
    return coefficients_.size() + ((constant_ != 0) ? 1 : 0);
  }

  static LinearCombination illegal() {
    LinearCombination l;
    l.illegal_ = true;
    return l;
  }

  LinearCombination &operator+(const LinearCombination &other) {
    if (illegal_ || other.illegal_) {
      illegal_ = true;
      return *this;
    }
    constant_ += other.constant_;
    for (auto &p : other.coefficients_) {
      coefficients_[p.first] += p.second;
    }
    trim();
    return *this;
  };

  LinearCombination &operator*(const LinearCombination &other) {
    if (illegal_ || other.illegal_) {
      illegal_ = true;
      return *this;
    }
    if (coefficients_.size() > 0 && other.coefficients_.size() > 0) {
      illegal_ = true;
      return *this;
    }
    auto mul_constant = constant_ * other.constant_;
    for (auto &p : coefficients_) {
      p.second *= other.constant_;
    }
    for (auto &p : other.coefficients_) {
      coefficients_[p.first] = p.second * constant_;
    }
    constant_ = mul_constant;
    trim();
    return *this;
  };

  std::optional<Operand> AsOperand() {
    if (illegal_)
      return std::nullopt;
    switch (coefficients_.size()) {
    case 0:
      return Operand::Mem(constant_);
    case 1: {
      auto c = *coefficients_.begin();
      if (auto s = Operand::ToScale(c.second))
        return Operand::Mem(*s, c.first, constant_);
      else
        return std::nullopt;
    }
    case 2: {
      auto it = coefficients_.cbegin();
      auto c1 = std::pair<X86Target::Reg, int>{*it++};
      auto c2 = std::pair<X86Target::Reg, int>{*it};
      if (c1.second > c2.second)
        std::swap(c1, c2);
      if (c1.second != 1)
        return std::nullopt;
      if (auto s = Operand::ToScale(c2.second))
        return Operand::Mem(c1.first, *s, c2.first, constant_);
      else
        return std::nullopt;
    }
    default:
      return std::nullopt;
    }
  }

private:
  bool illegal_ = false;
  std::int32_t constant_ = 0;
  std::map<X86Register, int> coefficients_; // invariant: nothing mapped to 0

  void trim() {
    for (auto it = coefficients_.begin(); it != coefficients_.end();) {
      if (it->second == 0)
        it = coefficients_.erase(it);
      else
        ++it;
    }
  }
};

class Muncher {
public:
  X86Prg Process(Tracer::TracedTreeProgram &prg) {
    std::vector<std::unique_ptr<X86Function>> functions;
    for (auto &f : prg.functions) {
      functions.push_back(function(f));
    }
    return {.functions = std::move(functions)};
  }

  using InstrVector = std::vector<std::unique_ptr<X86Instr>>;
  const InstrVector &GetCode() const { return code_; }

private:
  std::unique_ptr<X86Function> function(TreeFunction &fun) {
    code_.clear();
    emit(std::make_unique<UnaryInstr>(PUSH, EBP));
    emit(std::make_unique<BinaryInstr>(MOV, EBP, ESP));
    emit(std::make_unique<BinaryInstr>(SUB, ESP, Operand::FrameSize()));

    auto ebx_save = Operand::Reg(Temp{});
    auto esi_save = Operand::Reg(Temp{});
    auto edi_save = Operand::Reg(Temp{});

    emit(std::make_unique<BinaryInstr>(MOV, ebx_save, EBX));
    emit(std::make_unique<BinaryInstr>(MOV, esi_save, ESI));
    emit(std::make_unique<BinaryInstr>(MOV, edi_save, EDI));

    for (auto &s : fun.body) {
      stm(*s);
    }

    emit(
        std::make_unique<BinaryInstr>(MOV, EAX, Operand::Reg(fun.return_temp)));
    emit(std::make_unique<BinaryInstr>(MOV, EBX, ebx_save));
    emit(std::make_unique<BinaryInstr>(MOV, ESI, esi_save));
    emit(std::make_unique<BinaryInstr>(MOV, EDI, edi_save));
    emit(std::make_unique<BinaryInstr>(MOV, ESP, EBP));
    emit(std::make_unique<UnaryInstr>(POP, EBP));
    emit(std::make_unique<RetInstr>());

    return std::make_unique<X86Function>(fun.name, std::move(code_));
  }

  void stm(TreeStm &stm) { StmMuncher{*this}.Visit(stm); }

  Operand exp(TreeExp &exp) {
    auto lc = LCMuncher{}.Visit(exp);
    auto o = lc.AsOperand();
    if (o) {
      auto n = lc.NumberOfSummands();
      if (1 < n && n < 3) {
        auto t = Operand::Reg(Temp{});
        emit(std::make_unique<BinaryInstr>(LEA, t, *o));
        return t;
      }
    }
    return ExpMuncher{*this}.Visit(exp);
  }

  Operand lexp(TreeExp &exp) { return LExpMuncher{*this}.Visit(exp); }

  Operand effective_address(TreeExp &e) {
    if (auto ea = LCMuncher{}.Visit(e).AsOperand()) {
      return *ea;
    } else {
      auto o = exp(e);
      auto t = Temp{};
      emit(std::make_unique<BinaryInstr>(MOV, Operand::Reg(t), o));
      return Operand::Mem(t);
    }
  }

  class StmMuncher : public TreeStmVisitor<void> {
  public:
    StmMuncher(Muncher &muncher) : muncher_(muncher) {}

  private:
    Muncher &muncher_;

    void emit(std::unique_ptr<X86Instr> i) { muncher_.emit(std::move(i)); }

    virtual void VisitMove(TreeStmMove &s) {
      auto l = muncher_.lexp(*s.GetDst());
      auto r = muncher_.exp(*s.GetSrc());
      if (l.IsReg() && r.IsImm() && r.GetImm() == 0) {
        emit(std::make_unique<BinaryInstr>(XOR, l, l));
      } else if (l.IsMem() && r.IsMem()) {
        auto t = Operand::Reg(Temp{});
        emit(std::make_unique<BinaryInstr>(MOV, t, r));
        emit(std::make_unique<BinaryInstr>(MOV, l, t));
      } else {
        emit(std::make_unique<BinaryInstr>(MOV, l, r));
      }
    };

    virtual void VisitJump(TreeStmJump &s) {
      if (s.GetTarget()->GetOp() == TreeExp::Op::TreeExpNameOp) {
        auto t = static_cast<TreeExpName &>(*s.GetTarget());
        emit(std::make_unique<JmpInstr>(t.GetName()));
      } else {
        assert(false);
      }
    };

    virtual void VisitCJump(TreeStmCJump &s) {
      JInstr::Kind cond;
      switch (s.GetRel()) {
      case TreeStmCJump::EQ:
        cond = JInstr::Kind::E;
        break;
      case TreeStmCJump::NE:
        cond = JInstr::Kind::NE;
        break;
      case TreeStmCJump::LT:
        cond = JInstr::Kind::L;
        break;
      case TreeStmCJump::GT:
        cond = JInstr::Kind::G;
        break;
      case TreeStmCJump::LE:
        cond = JInstr::Kind::LE;
        break;
      case TreeStmCJump::GE:
        cond = JInstr::Kind::GE;
        break;
      default:
        assert(false);
        abort();
      }
      auto l = muncher_.exp(*s.GetLeft());
      auto r = muncher_.exp(*s.GetRight());
      if (l.IsImm()) {
        auto t = Operand::Reg(Temp{});
        emit(std::make_unique<BinaryInstr>(MOV, t, l));
        emit(std::make_unique<BinaryInstr>(CMP, t, r));
      } else if (l.IsMem() && r.IsMem()) {
        auto t = Operand::Reg(Temp{});
        emit(std::make_unique<BinaryInstr>(MOV, t, l));
        emit(std::make_unique<BinaryInstr>(CMP, t, r));
      } else {
        emit(std::make_unique<BinaryInstr>(CMP, l, r));
      }
      emit(std::make_unique<JInstr>(cond, s.GetLTrue()));
    };

    virtual void VisitLabel(TreeStmLabel &s) {
      emit(std::make_unique<LabelInstr>(s.GetLabel()));
    };

    virtual void VisitSeq(TreeStmSeq &s) {
      for (auto &si : s.GetTreeStms()) {
        Visit(*si);
      }
    };
  };

  class ExpMuncher : public TreeExpVisitor<Operand> {
  public:
    ExpMuncher(Muncher &muncher) : muncher_(muncher) {}

  private:
    Muncher &muncher_;

    void emit(std::unique_ptr<X86Instr> i) { muncher_.emit(std::move(i)); }

    virtual Operand VisitConst(TreeExpConst &e) {
      return Operand::Imm(e.GetValue());
    };

    virtual Operand VisitName(TreeExpName &e) {
      assert(false);
      abort();
    };

    virtual Operand VisitTemp(TreeExpTemp &e) {
      return Operand::Reg(e.GetTemp());
    };

    virtual Operand VisitParam(TreeExpParam &e) {
      return Operand::Mem(EBP, (std::int32_t)(8 + 4 * e.GetNumber()));
    };

    virtual Operand VisitMem(TreeExpMem &e) {
      return muncher_.effective_address(*e.GetAddr());
    };

    virtual Operand VisitBinOp(TreeExpBinOp &e) {
      auto l = muncher_.exp(*e.GetLeft());
      auto r = muncher_.exp(*e.GetRight());
      auto generic = [this](auto o, auto l, auto r) {
        auto t = Operand::Reg(Temp{});
        emit(std::make_unique<BinaryInstr>(MOV, t, l));
        emit(std::make_unique<BinaryInstr>(o, t, r));
        return t;
      };
      switch (e.GetBinOp()) {
      case TreeExpBinOp::PLUS:
        return generic(ADD, l, r);
      case TreeExpBinOp::MINUS:
        return generic(SUB, l, r);
      case TreeExpBinOp::MUL:
        return generic(IMUL, l, r);
      case TreeExpBinOp::DIV: {
        auto t = Operand::Reg(Temp{});
        emit(std::make_unique<BinaryInstr>(MOV, EAX, l));
        emit(std::make_unique<BinaryInstr>(MOV, EDX, EAX));
        emit(std::make_unique<BinaryInstr>(SAR, EDX, Operand::Imm(31)));
        if (r.IsImm()) {
          auto s = Operand::Reg(Temp{});
          emit(std::make_unique<BinaryInstr>(MOV, s, r));
          emit(std::make_unique<UnaryInstr>(IDIV, s));
        } else {
          emit(std::make_unique<UnaryInstr>(IDIV, r));
        }
        emit(std::make_unique<BinaryInstr>(MOV, t, EAX));
        return t;
      }
      case TreeExpBinOp::AND:
        return generic(AND, l, r);
      case TreeExpBinOp::OR:
        return generic(OR, l, r);
      case TreeExpBinOp::LSHIFT:
        return generic(SHL, l, r);
      case TreeExpBinOp::RSHIFT:
        return generic(SHR, l, r);
      case TreeExpBinOp::ARSHIFT:
        return generic(SAR, l, r);
      case TreeExpBinOp::XOR:
        return generic(XOR, l, r);
      default:
        assert(false);
        abort();
      }
    };

    virtual Operand VisitCall(TreeExpCall &e) {
      if (e.GetFun()->GetOp() == TreeExp::TreeExpNameOp) {
        auto f = static_cast<TreeExpName &>(*e.GetFun());
        for (auto rit = e.GetArgs().rbegin(); rit != e.GetArgs().rend();
             ++rit) {
          auto o = muncher_.exp(**rit);
          emit(std::make_unique<UnaryInstr>(PUSH, o));
        }
        emit(std::make_unique<CallInstr>(f.GetName()));
        auto t = Operand::Reg(Temp{});
        emit(std::make_unique<BinaryInstr>(MOV, t, EAX));
        emit(std::make_unique<BinaryInstr>(
            ADD, ESP, Operand::Imm(X86Target::WORD_SIZE * e.GetArgs().size())));
        return t;
      } else {
        assert(false);
        abort();
      }
    };
    virtual Operand VisitESeq(TreeExpESeq &e) {
      assert(false);
      abort();
    };
  };

  class LExpMuncher : public TreeExpVisitor<Operand> {
  public:
    LExpMuncher(Muncher &muncher) : muncher_(muncher) {}

  private:
    Muncher &muncher_;

    virtual Operand VisitConst(TreeExpConst &e) {
      assert(false);
      abort();
    };

    virtual Operand VisitName(TreeExpName &e) {
      assert(false);
      abort();
    };

    virtual Operand VisitTemp(TreeExpTemp &e) {
      return Operand::Reg(e.GetTemp());
    };

    virtual Operand VisitParam(TreeExpParam &e) {
      return Operand::Mem(EBP, (std::int32_t)(8 + 4 * e.GetNumber()));
    };

    virtual Operand VisitMem(TreeExpMem &e) {
      return muncher_.effective_address(*e.GetAddr());
    };

    virtual Operand VisitBinOp(TreeExpBinOp &e) {
      assert(false);
      abort();
    };

    virtual Operand VisitCall(TreeExpCall &e) {
      assert(false);
      abort();
    };

    virtual Operand VisitESeq(TreeExpESeq &e) {
      assert(false);
      abort();
    };
  };

  class LCMuncher : public TreeExpVisitor<LinearCombination> {
  public:
    virtual LinearCombination VisitConst(TreeExpConst &e) {
      return e.GetValue();
    }

    virtual LinearCombination VisitName(TreeExpName &e) {
      return LinearCombination::illegal();
    }

    virtual LinearCombination VisitTemp(TreeExpTemp &e) { return e.GetTemp(); }

    virtual LinearCombination VisitParam(TreeExpParam &e) {
      return LinearCombination::illegal();
    };

    virtual LinearCombination VisitMem(TreeExpMem &e) {
      return LinearCombination::illegal();
    };

    virtual LinearCombination VisitBinOp(TreeExpBinOp &e) {
      switch (e.GetBinOp()) {
      case TreeExpBinOp::PLUS:
        return Visit(*e.GetLeft()) + Visit(*e.GetRight());
      case TreeExpBinOp::MUL:
        return Visit(*e.GetLeft()) * Visit(*e.GetRight());
      case TreeExpBinOp::MINUS:
        return Visit(*e.GetLeft()) +
               LinearCombination(-1) * Visit(*e.GetRight());
      default:
        return LinearCombination::illegal();
      }
    };

    virtual LinearCombination VisitCall(TreeExpCall &e) {
      return LinearCombination::illegal();
    };

    virtual LinearCombination VisitESeq(TreeExpESeq &e) {
      assert(false);
      abort();
    };
  };

  InstrVector code_;

  void emit(std::unique_ptr<X86Instr> i) { code_.push_back(std::move(i)); }
};

X86Prg X86Target::CodeGen(Tracer::TracedTreeProgram &prg) {
  return Muncher{}.Process(prg);
}

} // namespace mjc
