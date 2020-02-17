#ifndef MJC_INTERMEDIATE_TREE_STM_H
#define MJC_INTERMEDIATE_TREE_STM_H

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "intermediate/names.h"
#include "intermediate/tree_exp.h"

namespace mjc {

// Representation of tree statements
class TreeStm {
public:
  enum Op {
    TreeStmMoveOp,
    TreeStmJumpOp,
    TreeStmCJumpOp,
    TreeStmLabelOp,
    TreeStmSeqOp
  };

  virtual ~TreeStm() {}

  virtual const Op GetOp() const = 0;
};

std::ostream &operator<<(std::ostream &os, TreeStm &stm);

class TreeStmMove : public TreeStm {
public:
  explicit TreeStmMove(std::unique_ptr<TreeExp> dst,
                       std::unique_ptr<TreeExp> src);

  virtual const Op GetOp() const;
  std::unique_ptr<TreeExp> &GetSrc();
  std::unique_ptr<TreeExp> &GetDst();

private:
  std::unique_ptr<TreeExp> src_;
  std::unique_ptr<TreeExp> dst_;
};

class TreeStmJump : public TreeStm {
public:
  explicit TreeStmJump(std::unique_ptr<TreeExp> target,
                       std::vector<Label> targets);
  explicit TreeStmJump(Label label);

  virtual const Op GetOp() const;
  std::unique_ptr<TreeExp> &GetTarget();
  const std::vector<Label> &GetTargets() const;

private:
  std::unique_ptr<TreeExp> target_;
  const std::vector<Label> targets_;
};

class TreeStmCJump : public TreeStm {
public:
  enum RelOp { EQ, NE, LT, GT, LE, GE, ULT, ULE, UGT, UGE };

  static RelOp negate(RelOp op);

  explicit TreeStmCJump(RelOp rel, std::unique_ptr<TreeExp> left,
                        std::unique_ptr<TreeExp> right, Label l_true,
                        Label l_false);

  virtual const Op GetOp() const;
  const RelOp &GetRel() const;
  std::unique_ptr<TreeExp> &GetLeft();
  std::unique_ptr<TreeExp> &GetRight();
  const Label &GetLTrue() const;
  const Label &GetLFalse() const;

private:
  const RelOp rel_;
  std::unique_ptr<TreeExp> left_;
  std::unique_ptr<TreeExp> right_;
  const Label l_true_;
  const Label l_false_;
};

std::ostream &operator<<(std::ostream &os, const TreeStmCJump::RelOp &relop);

class TreeStmLabel : public TreeStm {
public:
  explicit TreeStmLabel(Label label);

  virtual const Op GetOp() const;
  const Label &GetLabel() const;

private:
  const Label label_;
};
class TreeStmSeq : public TreeStm {
public:
  explicit TreeStmSeq(std::vector<std::unique_ptr<TreeStm>> stms);

  virtual const Op GetOp() const;
  const std::vector<std::unique_ptr<TreeStm>> &GetTreeStms() const;

private:
  const std::vector<std::unique_ptr<TreeStm>> stms_;
};

template <typename RetTy> class TreeStmVisitor {
public:
  virtual RetTy VisitMove(TreeStmMove &s) = 0;
  virtual RetTy VisitJump(TreeStmJump &s) = 0;
  virtual RetTy VisitCJump(TreeStmCJump &s) = 0;
  virtual RetTy VisitLabel(TreeStmLabel &s) = 0;
  virtual RetTy VisitSeq(TreeStmSeq &s) = 0;

  RetTy Visit(TreeStm &s) {
    switch (s.GetOp()) {
    case TreeStm::TreeStmMoveOp:
      return VisitMove(static_cast<TreeStmMove &>(s));
    case TreeStm::TreeStmJumpOp:
      return VisitJump(static_cast<TreeStmJump &>(s));
    case TreeStm::TreeStmCJumpOp:
      return VisitCJump(static_cast<TreeStmCJump &>(s));
    case TreeStm::TreeStmLabelOp:
      return VisitLabel(static_cast<TreeStmLabel &>(s));
    case TreeStm::TreeStmSeqOp:
      return VisitSeq(static_cast<TreeStmSeq &>(s));
    }
    assert(false);
    return RetTy();
  }
};
} // namespace mjc

#endif
