#include "minijava/exp.h"

namespace mjc {

void Exp::SetLocation(location l) { location_ = l; }

std::optional<location> Exp::GetLocation() const { return location_; }

ExpNum::ExpNum(int32_t num) : num{num} {}

const Exp::Op ExpNum::GetOp() const { return ExpNumOp; }

const int32_t ExpNum::GetNum() const { return num; }

ExpId::ExpId(std::string id) : id_{id} {}

const Exp::Op ExpId::GetOp() const { return ExpIdOp; }

const std::string &ExpId::GetId() const { return id_; }

ExpBinOp::ExpBinOp(std::shared_ptr<Exp> left, BinOp binop,
                   std::shared_ptr<Exp> right)
    : left_(std::move(left)), op_(binop), right_(std::move(right)) {
  assert(left_);
  assert(right_);
}

const Exp::Op ExpBinOp::GetOp() const { return ExpBinOpOp; }

const Exp &ExpBinOp::GetLeft() const { return *left_; }

const ExpBinOp::BinOp ExpBinOp::GetBinOp() const { return op_; }

const Exp &ExpBinOp::GetRight() const { return *right_; }

ExpInvoke::ExpInvoke(std::shared_ptr<Exp> obj, std::string fun,
                     std::vector<std::shared_ptr<Exp>> args)
    : obj_(std::move(obj)), method_(fun), args_(std::move(args)) {
  assert(obj_);
  for (auto arg : args) {
    assert(arg);
  }
}

const Exp::Op ExpInvoke::GetOp() const { return ExpInvokeOp; }

const Exp &ExpInvoke::GetObj() const { return *obj_; }

const std::string &ExpInvoke::GetMethod() const { return method_; }

const std::vector<std::shared_ptr<Exp>> &ExpInvoke::GetArgs() const {
  return args_;
}

ExpArrayGet::ExpArrayGet(std::shared_ptr<Exp> array, std::shared_ptr<Exp> index)
    : array_(std::move(array)), index_(std::move(index)) {
  assert(array_);
  assert(index_);
}

const Exp::Op ExpArrayGet::GetOp() const { return ExpArrayGetOp; }

const Exp &ExpArrayGet::GetArray() const { return *array_; }

const Exp &ExpArrayGet::GetIndex() const { return *index_; }

ExpArrayLength::ExpArrayLength(std::shared_ptr<Exp> array)
    : array_(std::move(array)) {
  assert(array_);
}

const Exp::Op ExpArrayLength::GetOp() const { return ExpArrayLengthOp; }

const Exp &ExpArrayLength::GetArray() const { return *array_; }

ExpTrue::ExpTrue() {}

const Exp::Op ExpTrue::GetOp() const { return ExpTrueOp; }

ExpFalse::ExpFalse() {}

const Exp::Op ExpFalse::GetOp() const { return ExpFalseOp; }

ExpThis::ExpThis() {}

const Exp::Op ExpThis::GetOp() const { return ExpThisOp; }

ExpNew::ExpNew(std::string cls) : cls_(cls) {}

const Exp::Op ExpNew::GetOp() const { return ExpNewOp; }

const std::string &ExpNew::GetCls() const { return cls_; }

ExpNewIntArray::ExpNewIntArray(std::shared_ptr<Exp> size)
    : size_(std::move(size)) {
  assert(size_);
}

const Exp::Op ExpNewIntArray::GetOp() const { return ExpNewIntArrayOp; }

const Exp &ExpNewIntArray::GetSize() const { return *size_; }

ExpNeg::ExpNeg(std::shared_ptr<Exp> exp) : exp_(std::move(exp)) {
  assert(exp_);
}

const Exp::Op ExpNeg::GetOp() const { return ExpNegOp; }

const Exp &ExpNeg::GetExp() const { return *exp_; }

ExpRead::ExpRead() {}

const Exp::Op ExpRead::GetOp() const { return ExpReadOp; }

}