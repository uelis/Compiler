#ifndef MJC_INTERMEDIATE_TREE_EXP_H
#define MJC_INTERMEDIATE_TREE_EXP_H

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "intermediate/names.h"

namespace mjc {

class TreeStm;

// Representation of tree expressions
class TreeExp {
public:
  enum Op {
    TreeExpConstOp,
    TreeExpNameOp,
    TreeExpTempOp,
    TreeExpParamOp,
    TreeExpMemOp,
    TreeExpBinOpOp,
    TreeExpCallOp,
    TreeExpESeqOp
  };

  virtual ~TreeExp() {}

  virtual const Op GetOp() const = 0;
};

std::ostream &operator<<(std::ostream &os, TreeExp &type);

class TreeExpConst : public TreeExp {
public:
  explicit TreeExpConst(int32_t value);

  virtual const Op GetOp() const;
  const int32_t GetValue() const;

private:
  int32_t value_;
};

class TreeExpName : public TreeExp {
public:
  explicit TreeExpName(Label name);

  virtual const Op GetOp() const;
  const Label &GetName() const;

private:
  const Label name_;
};

class TreeExpTemp : public TreeExp {
public:
  explicit TreeExpTemp(Temp temp);

  virtual const Op GetOp() const;
  const Temp &GetTemp() const;

private:
  const Temp temp_;
};

class TreeExpParam : public TreeExp {
public:
  explicit TreeExpParam(int32_t num);

  virtual const Op GetOp() const;
  const int32_t GetNumber() const;

private:
  int32_t number_;
};

class TreeExpBinOp : public TreeExp {
public:
  enum BinOp { PLUS, MINUS, MUL, DIV, AND, OR, LSHIFT, RSHIFT, ARSHIFT, XOR };

  explicit TreeExpBinOp(BinOp binop, std::unique_ptr<TreeExp> left,
                        std::unique_ptr<TreeExp> right);

  virtual const Op GetOp() const;
  std::unique_ptr<TreeExp> &GetLeft();
  const BinOp &GetBinOp() const;
  std::unique_ptr<TreeExp> &GetRight();

private:
  std::unique_ptr<TreeExp> left_;
  BinOp op_;
  std::unique_ptr<TreeExp> right_;
};

std::ostream &operator<<(std::ostream &os, TreeExpBinOp::BinOp type);

class TreeExpCall : public TreeExp {
public:
  explicit TreeExpCall(std::unique_ptr<TreeExp> fun,
                       std::vector<std::unique_ptr<TreeExp>> args);
  explicit TreeExpCall(Label fun, std::unique_ptr<TreeExp> arg);

  virtual const Op GetOp() const;
  std::unique_ptr<TreeExp> &GetFun();
  std::vector<std::unique_ptr<TreeExp>> &GetArgs();

private:
  std::unique_ptr<TreeExp> fun_;
  std::vector<std::unique_ptr<TreeExp>> args_;
};

class TreeExpMem : public TreeExp {
public:
  explicit TreeExpMem(std::unique_ptr<TreeExp> addr);

  virtual const Op GetOp() const;
  std::unique_ptr<TreeExp> &GetAddr();

private:
  std::unique_ptr<TreeExp> addr_;
};

class TreeExpESeq : public TreeExp {
public:
  explicit TreeExpESeq(std::vector<std::unique_ptr<TreeStm>> stms,
                       std::unique_ptr<TreeExp> exp);

  virtual const Op GetOp() const;
  std::vector<std::unique_ptr<TreeStm>> &GetStms();
  std::unique_ptr<TreeExp> &GetExp();

private:
  std::vector<std::unique_ptr<TreeStm>> stms_;
  std::unique_ptr<TreeExp> exp_;
};

template <typename RetTy> class TreeExpVisitor {
public:
  virtual RetTy VisitConst(TreeExpConst &e) = 0;
  virtual RetTy VisitName(TreeExpName &e) = 0;
  virtual RetTy VisitTemp(TreeExpTemp &e) = 0;
  virtual RetTy VisitParam(TreeExpParam &e) = 0;
  virtual RetTy VisitMem(TreeExpMem &e) = 0;
  virtual RetTy VisitBinOp(TreeExpBinOp &e) = 0;
  virtual RetTy VisitCall(TreeExpCall &e) = 0;
  virtual RetTy VisitESeq(TreeExpESeq &e) = 0;

  RetTy Visit(TreeExp &exp) {
    switch (exp.GetOp()) {
    case TreeExp::TreeExpConstOp:
      return VisitConst(static_cast<TreeExpConst &>(exp));
    case TreeExp::TreeExpNameOp:
      return VisitName(static_cast<TreeExpName &>(exp));
    case TreeExp::TreeExpTempOp:
      return VisitTemp(static_cast<TreeExpTemp &>(exp));
    case TreeExp::TreeExpParamOp:
      return VisitParam(static_cast<TreeExpParam &>(exp));
    case TreeExp::TreeExpMemOp:
      return VisitMem(static_cast<TreeExpMem &>(exp));
    case TreeExp::TreeExpBinOpOp:
      return VisitBinOp(static_cast<TreeExpBinOp &>(exp));
    case TreeExp::TreeExpCallOp:
      return VisitCall(static_cast<TreeExpCall &>(exp));
    case TreeExp::TreeExpESeqOp:
      return VisitESeq(static_cast<TreeExpESeq &>(exp));
    }
    assert(false);
    abort();
  }
};
} // namespace mjc

#endif
