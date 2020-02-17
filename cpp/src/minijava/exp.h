#ifndef MJC_MINIJAVA_EXP_H
#define MJC_MINIJAVA_EXP_H

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "minijava/location.hh"

namespace mjc {

// Represents MiniJava expressions
class Exp {
 public:
  enum Op {
    ExpNumOp,
    ExpIdOp,
    ExpBinOpOp,
    ExpInvokeOp,
    ExpArrayGetOp,
    ExpArrayLengthOp,
    ExpTrueOp,
    ExpFalseOp,
    ExpThisOp,
    ExpNewOp,
    ExpNewIntArrayOp,
    ExpNegOp,
    ExpReadOp
  };

  virtual ~Exp() {}

  virtual const Op GetOp() const = 0;
  void SetLocation(location l);
  std::optional<location> GetLocation() const;

 private:
  std::optional<location> location_;
};

class ExpNum : public Exp {
 public:
  explicit ExpNum(int32_t num);

  virtual const Op GetOp() const;
  const int32_t GetNum() const;

 private:
  int32_t num;
};

class ExpId : public Exp {
 public:
  explicit ExpId(std::string id);

  virtual const Op GetOp() const;
  const std::string &GetId() const;

 private:
  const std::string id_;
};

class ExpBinOp : public Exp {
 public:
  enum BinOp { PLUS, MINUS, MUL, DIV, LT, STRICTAND };

  explicit ExpBinOp(std::shared_ptr<Exp> left, BinOp binop,
                    std::shared_ptr<Exp> right);

  virtual const Op GetOp() const;
  const Exp &GetLeft() const;
  const BinOp GetBinOp() const;
  const Exp &GetRight() const;

 private:
  const std::shared_ptr<Exp> left_;
  const BinOp op_;
  const std::shared_ptr<Exp> right_;
};

class ExpInvoke : public Exp {
 public:
  explicit ExpInvoke(std::shared_ptr<Exp> obj, std::string fun,
                     std::vector<std::shared_ptr<Exp>> args);

  virtual const Op GetOp() const;
  const Exp &GetObj() const;
  const std::string &GetMethod() const;
  const std::vector<std::shared_ptr<Exp>> &GetArgs() const;

 private:
  const std::shared_ptr<Exp> obj_;
  const std::string method_;
  const std::vector<std::shared_ptr<Exp>> args_;
};

class ExpArrayGet : public Exp {
 public:
  explicit ExpArrayGet(std::shared_ptr<Exp> array, std::shared_ptr<Exp> index);

  virtual const Op GetOp() const;
  const Exp &GetArray() const;
  const Exp &GetIndex() const;

 private:
  const std::shared_ptr<Exp> array_;
  const std::shared_ptr<Exp> index_;
};

class ExpArrayLength : public Exp {
 public:
  explicit ExpArrayLength(std::shared_ptr<Exp> array);

  virtual const Op GetOp() const;
  const Exp &GetArray() const;

 private:
  const std::shared_ptr<Exp> array_;
};

class ExpTrue : public Exp {
 public:
  explicit ExpTrue();

  virtual const Op GetOp() const;
};

class ExpFalse : public Exp {
 public:
  explicit ExpFalse();

  virtual const Op GetOp() const;
};

class ExpThis : public Exp {
 public:
  explicit ExpThis();

  virtual const Op GetOp() const;
};

class ExpNew : public Exp {
  const std::string cls_;

 public:
  explicit ExpNew(std::string cls);

  virtual const Op GetOp() const;
  const std::string &GetCls() const;
};

class ExpNewIntArray : public Exp {
 public:
  explicit ExpNewIntArray(std::shared_ptr<Exp> size);

  virtual const Op GetOp() const;
  const Exp &GetSize() const;

 private:
  const std::shared_ptr<Exp> size_;
};

class ExpNeg : public Exp {
 public:
  explicit ExpNeg(std::shared_ptr<Exp> exp);

  virtual const Op GetOp() const;
  const Exp &GetExp() const;

 private:
  const std::shared_ptr<Exp> exp_;
};

class ExpRead : public Exp {
 public:
  explicit ExpRead();

  virtual const Op GetOp() const;
};

template <typename RetTy>
class ExpVisitor {
 public:
  virtual RetTy VisitNum(const ExpNum &e) = 0;
  virtual RetTy VisitId(const ExpId &e) = 0;
  virtual RetTy VisitBinOp(const ExpBinOp &e) = 0;
  virtual RetTy VisitInvoke(const ExpInvoke &e) = 0;
  virtual RetTy VisitArrayGet(const ExpArrayGet &e) = 0;
  virtual RetTy VisitArrayLength(const ExpArrayLength &e) = 0;
  virtual RetTy VisitTrue(const ExpTrue &e) = 0;
  virtual RetTy VisitFalse(const ExpFalse &e) = 0;
  virtual RetTy VisitThis(const ExpThis &e) = 0;
  virtual RetTy VisitNew(const ExpNew &e) = 0;
  virtual RetTy VisitNewIntArray(const ExpNewIntArray &e) = 0;
  virtual RetTy VisitNeg(const ExpNeg &e) = 0;
  virtual RetTy VisitRead(const ExpRead &e) = 0;

  RetTy Visit(const Exp &exp) {
    switch (exp.GetOp()) {
      case Exp::ExpNumOp:
        return VisitNum(static_cast<const ExpNum &>(exp));
      case Exp::ExpIdOp:
        return VisitId(static_cast<const ExpId &>(exp));
      case Exp::ExpBinOpOp:
        return VisitBinOp(static_cast<const ExpBinOp &>(exp));
      case Exp::ExpInvokeOp:
        return VisitInvoke(static_cast<const ExpInvoke &>(exp));
      case Exp::ExpArrayGetOp:
        return VisitArrayGet(static_cast<const ExpArrayGet &>(exp));
      case Exp::ExpArrayLengthOp:
        return VisitArrayLength(static_cast<const ExpArrayLength &>(exp));
      case Exp::ExpTrueOp:
        return VisitTrue(static_cast<const ExpTrue &>(exp));
      case Exp::ExpFalseOp:
        return VisitFalse(static_cast<const ExpFalse &>(exp));
      case Exp::ExpThisOp:
        return VisitThis(static_cast<const ExpThis &>(exp));
      case Exp::ExpNewOp:
        return VisitNew(static_cast<const ExpNew &>(exp));
      case Exp::ExpNewIntArrayOp:
        return VisitNewIntArray(static_cast<const ExpNewIntArray &>(exp));
      case Exp::ExpNegOp:
        return VisitNeg(static_cast<const ExpNeg &>(exp));
      case Exp::ExpReadOp:
        return VisitRead(static_cast<const ExpRead &>(exp));
    }
    assert(false);
    abort();
  }
};
}  // namespace mjc

#endif
