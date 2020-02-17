#include "minijava/stm.h"

namespace mjc {

void Stm::SetLocation(location l) { location_ = l; }

std::optional<location> Stm::GetLocation() const { return location_; }

StmAssignment::StmAssignment(std::string id, std::shared_ptr<Exp> exp)
    : id_(id), exp_(std::move(exp)) {
  assert(exp_);
};

const Stm::Op StmAssignment::GetOp() const { return StmAssignmentOp; }

const std::string &StmAssignment::GetId() const { return id_; }
const Exp &StmAssignment::GetExp() const { return *exp_; }

StmArrayAssignment::StmArrayAssignment(std::string id,
                                       std::shared_ptr<Exp> index,
                                       std::shared_ptr<Exp> exp)
    : id_(id), index_(std::move(index)), exp_(std::move(exp)) {
  assert(index_);
  assert(exp_);
};

const Stm::Op StmArrayAssignment::GetOp() const { return StmArrayAssignmentOp; }

const std::string &StmArrayAssignment::GetId() const { return id_; }

const Exp &StmArrayAssignment::GetIndex() const { return *index_; }

const Exp &StmArrayAssignment::GetExp() const { return *exp_; }

StmIf::StmIf(std::shared_ptr<Exp> cond, std::shared_ptr<Stm> true_branch,
             std::shared_ptr<Stm> false_branch)
    : cond_(std::move(cond)),
      true_branch_(std::move(true_branch)),
      false_branch_(std::move(false_branch)) {
  assert(cond_);
  assert(true_branch_);
  assert(false_branch_);
};

const Stm::Op StmIf::GetOp() const { return StmIfOp; }

const Exp &StmIf::GetCond() const { return *cond_; }

const Stm &StmIf::GetTrueBranch() const { return *true_branch_; }

const Stm &StmIf::GetFalseBranch() const { return *false_branch_; }

StmWhile::StmWhile(std::shared_ptr<Exp> cond, std::shared_ptr<Stm> body)
    : cond_(std::move(cond)), body_(std::move(body)) {
  assert(cond_);
  assert(body_);
};

const Stm::Op StmWhile::GetOp() const { return StmWhileOp; }

const Exp &StmWhile::GetCond() const { return *cond_; }

const Stm &StmWhile::GetBody() const { return *body_; }

StmPrint::StmPrint(std::shared_ptr<Exp> exp) : exp_(std::move(exp)) {
  assert(exp_);
};

const Stm::Op StmPrint::GetOp() const { return StmPrintOp; }

const Exp &StmPrint::GetExp() const { return *exp_; }

StmWrite::StmWrite(std::shared_ptr<Exp> exp) : exp_(std::move(exp)) {
  assert(exp_);
};

const Stm::Op StmWrite::GetOp() const { return StmWriteOp; }

const Exp &StmWrite::GetExp() const { return *exp_; }

StmSeq::StmSeq(std::vector<std::shared_ptr<Stm>> stms)
    : stms_(std::move(stms)) {
  for (auto s : stms_) {
    assert(s);
  }
};

const Stm::Op StmSeq::GetOp() const { return StmSeqOp; }

const std::vector<std::shared_ptr<Stm>> &StmSeq::GetStms() const {
  return stms_;
}
}  // namespace mjc