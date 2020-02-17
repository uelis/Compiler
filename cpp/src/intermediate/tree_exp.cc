#include "intermediate/tree_exp.h"

#include <iostream>

#include "intermediate/tree_stm.h"

namespace mjc {

TreeExpConst::TreeExpConst(int32_t value) : value_{value} {}

const TreeExp::Op TreeExpConst::GetOp() const { return TreeExpConstOp; }

const int32_t TreeExpConst::GetValue() const { return value_; }

TreeExpName::TreeExpName(Label name) : name_{std::move(name)} {}

const TreeExp::Op TreeExpName::GetOp() const { return TreeExpNameOp; }

const Label &TreeExpName::GetName() const { return name_; }

TreeExpTemp::TreeExpTemp(Temp temp) : temp_{temp} {}

const TreeExp::Op TreeExpTemp::GetOp() const { return TreeExpTempOp; }

const Temp &TreeExpTemp::GetTemp() const { return temp_; }

TreeExpParam::TreeExpParam(int32_t num) : number_{num} {}

const TreeExp::Op TreeExpParam::GetOp() const { return TreeExpParamOp; }

const int32_t TreeExpParam::GetNumber() const { return number_; }

TreeExpBinOp::TreeExpBinOp(BinOp binop, std::unique_ptr<TreeExp> left,
                           std::unique_ptr<TreeExp> right)
    : left_(std::move(left)), op_(binop), right_(std::move(right)) {
  assert(left_);
  assert(right_);
}

const TreeExp::Op TreeExpBinOp::GetOp() const { return TreeExpBinOpOp; }

std::unique_ptr<TreeExp> &TreeExpBinOp::GetLeft() { return left_; }

const TreeExpBinOp::BinOp &TreeExpBinOp::GetBinOp() const { return op_; }

std::unique_ptr<TreeExp> &TreeExpBinOp::GetRight() { return right_; }

TreeExpCall::TreeExpCall(std::unique_ptr<TreeExp> fun,
                         std::vector<std::unique_ptr<TreeExp>> args)
    : fun_(std::move(fun)), args_(std::move(args)) {
  assert(fun_);
  assert(std::all_of(args.begin(), args.end(),
                     [](auto &arg) { return (bool)arg; }));
}

TreeExpCall::TreeExpCall(Label fun, std::unique_ptr<TreeExp> arg)
    : TreeExpCall(std::make_unique<TreeExpName>(std::move(fun)), {}) {
  args_.push_back(std::move(arg));
}

const TreeExp::Op TreeExpCall::GetOp() const { return TreeExpCallOp; }

std::unique_ptr<TreeExp> &TreeExpCall::GetFun() { return fun_; }

std::vector<std::unique_ptr<TreeExp>> &TreeExpCall::GetArgs() { return args_; }

TreeExpMem::TreeExpMem(std::unique_ptr<TreeExp> addr) : addr_(std::move(addr)) {
  assert(addr_);
}

const TreeExp::Op TreeExpMem::GetOp() const { return TreeExpMemOp; }

std::unique_ptr<TreeExp> &TreeExpMem::GetAddr() { return addr_; }

TreeExpESeq::TreeExpESeq(std::vector<std::unique_ptr<TreeStm>> stms,
                         std::unique_ptr<TreeExp> exp)
    : stms_(std::move(stms)), exp_(std::move(exp)) {
  assert(exp_);
  assert(
      std::all_of(stms_.begin(), stms_.end(), [](auto &x) { return (bool)x; }));
}

const TreeExp::Op TreeExpESeq::GetOp() const { return TreeExpESeqOp; }

std::vector<std::unique_ptr<TreeStm>> &TreeExpESeq::GetStms() { return stms_; }

std::unique_ptr<TreeExp> &TreeExpESeq::GetExp() { return exp_; }

class TreeExpOut : public TreeExpVisitor<void> {
 public:
  TreeExpOut(std::ostream &out) : out_(out) {}

  virtual void VisitConst(TreeExpConst &e) {
    out_ << "CONST(" << e.GetValue() << ")";
  }

  virtual void VisitName(TreeExpName &e) {
    out_ << "NAME(" << e.GetName() << ")";
  }

  virtual void VisitTemp(TreeExpTemp &e) {
    out_ << "TEMP(" << e.GetTemp() << ")";
  }

  virtual void VisitParam(TreeExpParam &e) {
    out_ << "PARAM(" << e.GetNumber() << ")";
  }

  virtual void VisitMem(TreeExpMem &e) {
    out_ << "MEM(" << *e.GetAddr() << ")";
  }

  virtual void VisitBinOp(TreeExpBinOp &e) {
    out_ << "BINOP(" << e.GetBinOp() << ", " << *e.GetLeft() << ", "
         << *e.GetRight() << ")";
  }

  virtual void VisitCall(TreeExpCall &e) {
    out_ << "CALL(" << *e.GetFun();
    for (auto const &arg : e.GetArgs()) {
      out_ << ", " << *arg;
    }
    out_ << ")";
  }

  virtual void VisitESeq(TreeExpESeq &e) {
    out_ << "ESEQ(";
    for (auto &s : e.GetStms()) {
      out_ << *s << ", ";
    }
    out_ << *e.GetExp() << ")";
  }

 private:
  std::ostream &out_;
};

std::ostream &operator<<(std::ostream &os, TreeExp &exp) {
  TreeExpOut(os).Visit(exp);
  return os;
}

std::ostream &operator<<(std::ostream &os, TreeExpBinOp::BinOp binop) {
  switch (binop) {
    case TreeExpBinOp::BinOp::PLUS:
      return os << "PLUS";
    case TreeExpBinOp::BinOp::MINUS:
      return os << "MINUS";
    case TreeExpBinOp::BinOp::MUL:
      return os << "MUL";
    case TreeExpBinOp::BinOp::DIV:
      return os << "DIV";
    case TreeExpBinOp::BinOp::AND:
      return os << "AND";
    case TreeExpBinOp::BinOp::OR:
      return os << "OR";
    case TreeExpBinOp::BinOp::LSHIFT:
      return os << "LSHIFT";
    case TreeExpBinOp::BinOp::RSHIFT:
      return os << "RSHIFT";
    case TreeExpBinOp::BinOp::ARSHIFT:
      return os << "ARSHIFT";
    case TreeExpBinOp::BinOp::XOR:
      return os << "XOR";
  }
  assert(false);
  return os;
}
}  // namespace mjc
