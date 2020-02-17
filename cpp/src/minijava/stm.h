//
// MiniJava statements
//

#ifndef MJC_MINIJAVA_STM_H
#define MJC_MINIJAVA_STM_H

#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "minijava/exp.h"
#include "minijava/location.hh"

namespace mjc {

// Represents MiniJava statements
class Stm {
 public:
  enum Op {
    StmAssignmentOp,
    StmArrayAssignmentOp,
    StmIfOp,
    StmWhileOp,
    StmPrintOp,
    StmWriteOp,
    StmSeqOp,
  };
  virtual ~Stm(){}

  virtual const Op GetOp() const = 0;
  void SetLocation(location l);
  std::optional<location> GetLocation() const;

 private:
  std::optional<location> location_;
};

class StmAssignment : public Stm {
 public:
  explicit StmAssignment(std::string id, std::shared_ptr<Exp> exp);

  virtual const Op GetOp() const;
  const std::string &GetId() const;
  const Exp &GetExp() const;

 private:
  const std::string id_;
  const std::shared_ptr<Exp> exp_;
};

class StmArrayAssignment : public Stm {
 public:
  explicit StmArrayAssignment(std::string id, std::shared_ptr<Exp> index,
                              std::shared_ptr<Exp> exp);

  virtual const Op GetOp() const;
  const std::string &GetId() const;
  const Exp &GetIndex() const;
  const Exp &GetExp() const;

 private:
  const std::string id_;
  const std::shared_ptr<Exp> index_;
  const std::shared_ptr<Exp> exp_;
};

class StmIf : public Stm {
 public:
  explicit StmIf(std::shared_ptr<Exp> cond, std::shared_ptr<Stm> true_branch,
                 std::shared_ptr<Stm> false_branch);

  virtual const Op GetOp() const;
  const Exp &GetCond() const;
  const Stm &GetTrueBranch() const;
  const Stm &GetFalseBranch() const;

 private:
  const std::shared_ptr<Exp> cond_;
  const std::shared_ptr<Stm> true_branch_;
  const std::shared_ptr<Stm> false_branch_;
};

class StmWhile : public Stm {
 public:
  explicit StmWhile(std::shared_ptr<Exp> cond, std::shared_ptr<Stm> body);

  virtual const Op GetOp() const;
  const Exp &GetCond() const;
  const Stm &GetBody() const;

 private:
  const std::shared_ptr<Exp> cond_;
  const std::shared_ptr<Stm> body_;
};

class StmPrint : public Stm {
 public:
  explicit StmPrint(std::shared_ptr<Exp> exp);

  virtual const Op GetOp() const;
  const Exp &GetExp() const;

 private:
  const std::shared_ptr<Exp> exp_;
};

class StmWrite : public Stm {
 public:
  explicit StmWrite(std::shared_ptr<Exp> exp);

  virtual const Op GetOp() const;
  const Exp &GetExp() const;

 private:
  const std::shared_ptr<Exp> exp_;
};

class StmSeq : public Stm {
 public:
  explicit StmSeq(std::vector<std::shared_ptr<Stm>> stms);

  virtual const Op GetOp() const;
  const std::vector<std::shared_ptr<Stm>> &GetStms() const;

 private:
  const std::vector<std::shared_ptr<Stm>> stms_;
};

template <typename RetTy>
class StmVisitor {
 public:
  virtual RetTy VisitAssignment(const StmAssignment &s) = 0;
  virtual RetTy VisitArrayAssignment(const StmArrayAssignment &s) = 0;
  virtual RetTy VisitIf(const StmIf &s) = 0;
  virtual RetTy VisitWhile(const StmWhile &s) = 0;
  virtual RetTy VisitPrint(const StmPrint &s) = 0;
  virtual RetTy VisitWrite(const StmWrite &s) = 0;
  virtual RetTy VisitSeq(const StmSeq &s) = 0;

  RetTy Visit(const Stm &s) {
    switch (s.GetOp()) {
      case Stm::StmAssignmentOp:
        return VisitAssignment(static_cast<const StmAssignment &>(s));
      case Stm::StmArrayAssignmentOp:
        return VisitArrayAssignment(static_cast<const StmArrayAssignment &>(s));
      case Stm::StmIfOp:
        return VisitIf(static_cast<const StmIf &>(s));
      case Stm::StmWhileOp:
        return VisitWhile(static_cast<const StmWhile &>(s));
      case Stm::StmPrintOp:
        return VisitPrint(static_cast<const StmPrint &>(s));
      case Stm::StmWriteOp:
        return VisitWrite(static_cast<const StmWrite &>(s));
      case Stm::StmSeqOp:
        return VisitSeq(static_cast<const StmSeq &>(s));
    }
    assert(false);
    return RetTy();
  }
};
}

#endif
