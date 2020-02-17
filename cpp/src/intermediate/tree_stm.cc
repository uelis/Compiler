#include "intermediate/tree_stm.h"

#include <iostream>
#include <string>

#include "intermediate/tree_exp.h"

namespace mjc {

TreeStmMove::TreeStmMove(std::unique_ptr<TreeExp> dst,
                         std::unique_ptr<TreeExp> src)
    : src_(std::move(src)), dst_(std::move(dst)) {
  assert(src_);
  assert(dst_);
};

const TreeStm::Op TreeStmMove::GetOp() const { return TreeStmMoveOp; }

std::unique_ptr<TreeExp> &TreeStmMove::GetSrc() { return src_; }

std::unique_ptr<TreeExp> &TreeStmMove::GetDst() { return dst_; }

TreeStmJump::TreeStmJump(std::unique_ptr<TreeExp> target,
                         std::vector<Label> targets)
    : target_(std::move(target)), targets_(std::move(targets)) {
  assert(target_);
};

TreeStmJump::TreeStmJump(Label label)
    : TreeStmJump(std::make_unique<TreeExpName>(label), {label}) {}

const TreeStm::Op TreeStmJump::GetOp() const { return TreeStmJumpOp; }

std::unique_ptr<TreeExp> &TreeStmJump::GetTarget() { return target_; }

const std::vector<Label> &TreeStmJump::GetTargets() const { return targets_; }

TreeStmCJump::TreeStmCJump(RelOp rel, std::unique_ptr<TreeExp> left,
                           std::unique_ptr<TreeExp> right, Label l_true,
                           Label l_false)
    : rel_(rel),
      left_(std::move(left)),
      right_(std::move(right)),
      l_true_(l_true),
      l_false_(l_false) {
  assert(left_);
  assert(right_);
};

const TreeStm::Op TreeStmCJump::GetOp() const { return TreeStmCJumpOp; }

const TreeStmCJump::RelOp &TreeStmCJump::GetRel() const { return rel_; }

std::unique_ptr<TreeExp> &TreeStmCJump::GetLeft() { return left_; }

std::unique_ptr<TreeExp> &TreeStmCJump::GetRight() { return right_; }

const Label &TreeStmCJump::GetLTrue() const { return l_true_; }

const Label &TreeStmCJump::GetLFalse() const { return l_false_; }

TreeStmCJump::RelOp TreeStmCJump::negate(TreeStmCJump::RelOp r) {
  switch (r) {
    case EQ:
      return NE;
    case NE:
      return EQ;
    case LT:
      return GE;
    case GT:
      return LE;
    case LE:
      return GT;
    case GE:
      return LT;
    case ULT:
      return UGE;
    case ULE:
      return UGT;
    case UGT:
      return ULE;
    case UGE:
      return ULT;
  }
  assert(false);
  abort();
}

TreeStmLabel::TreeStmLabel(Label label) : label_(std::move(label)){};

const TreeStm::Op TreeStmLabel::GetOp() const { return TreeStmLabelOp; }

const Label &TreeStmLabel::GetLabel() const { return label_; }

TreeStmSeq::TreeStmSeq(std::vector<std::unique_ptr<TreeStm>> stms)
    : stms_(std::move(stms)) {
  assert(
      std::all_of(stms_.begin(), stms_.end(), [](auto &x) { return (bool)x; }));
};

const TreeStm::Op TreeStmSeq::GetOp() const { return TreeStmSeqOp; }

const std::vector<std::unique_ptr<TreeStm>> &TreeStmSeq::GetTreeStms() const {
  return stms_;
}

class TreeStmOut : public TreeStmVisitor<void> {
 public:
  TreeStmOut(std::ostream &out) : out_(out) {}

  virtual void VisitMove(TreeStmMove &s) {
    out_ << "MOVE(" << *s.GetDst() << ", " << *s.GetSrc() << ")";
  }

  virtual void VisitJump(TreeStmJump &s) {
    out_ << "JUMP(" << *s.GetTarget();
    for (auto &arg : s.GetTargets()) {
      out_ << ", " << arg;
    }
    out_ << ")";
  }
  virtual void VisitCJump(TreeStmCJump &s) {
    out_ << "CJUMP(" << s.GetRel() << ", " << *s.GetLeft() << ","
         << *s.GetRight() << "," << s.GetLTrue() << "," << s.GetLFalse() << ")";
  }
  virtual void VisitLabel(TreeStmLabel &s) {
    out_ << "LABEL(" << s.GetLabel() << ")";
  }

  virtual void VisitSeq(TreeStmSeq &s) {
    out_ << "SEQ(";
    std::string sep = "";
    for (auto &s : s.GetTreeStms()) {
      out_ << sep << *s;
      sep = ", ";
    }
    out_ << ")";
  }

 private:
  std::ostream &out_;
};

std::ostream &operator<<(std::ostream &os, TreeStm &stm) {
  TreeStmOut(os).Visit(stm);
  return os;
}

std::ostream &operator<<(std::ostream &os, const TreeStmCJump::RelOp &relop) {
  switch (relop) {
    case TreeStmCJump::RelOp::EQ:
      return os << "EQ";
    case TreeStmCJump::RelOp::NE:
      return os << "NE";
    case TreeStmCJump::RelOp::LT:
      return os << "LT";
    case TreeStmCJump::RelOp::GT:
      return os << "GT";
    case TreeStmCJump::RelOp::LE:
      return os << "LE";
    case TreeStmCJump::RelOp::GE:
      return os << "GE";
    case TreeStmCJump::RelOp::ULT:
      return os << "ULT";
    case TreeStmCJump::RelOp::ULE:
      return os << "ULE";
    case TreeStmCJump::RelOp::UGT:
      return os << "UGT";
    case TreeStmCJump::RelOp::UGE:
      return os << "UGE";
  }
  assert(false);
  return os;
}
}  // namespace mjc
