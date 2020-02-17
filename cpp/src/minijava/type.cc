#include "minijava/type.h"

#include <iostream>


namespace mjc {

void Type::SetLocation(location l) { location_ = l; }

std::optional<location> Type::GetLocation() const { return location_; }

bool Type::operator==(const Type &other) const {
  return other.GetOp() == GetOp();
};

bool Type::operator!=(const Type &other) const { return !(*this == other); }

TypeVoid::TypeVoid() {}

const Type::Op TypeVoid::GetOp() const { return TypeVoidOp; }

bool TypeVoid::operator==(const Type &other) const {
  return other.GetOp() == GetOp();
};

TypeInt::TypeInt() {}

const Type::Op TypeInt::GetOp() const { return TypeIntOp; }

bool TypeInt::operator==(const Type &other) const {
  return other.GetOp() == GetOp();
};

TypeBool::TypeBool() {}

const Type::Op TypeBool::GetOp() const { return TypeBoolOp; }

bool TypeBool::operator==(const Type &other) const {
  return other.GetOp() == GetOp();
};

TypeArray::TypeArray(std::shared_ptr<Type> elem) : elem_(std::move(elem)) {
  assert(elem_);
};

const Type::Op TypeArray::GetOp() const { return TypeArrayOp; }

const Type &TypeArray::GetElem() const { return *elem_; }

bool TypeArray::operator==(const Type &other) const {
  return (other.GetOp() == GetOp()) &&
         *elem_ == *static_cast<const TypeArray &>(other).elem_;
};

TypeClass::TypeClass(std::string name) : name_(std::move(name)){};

const Type::Op TypeClass::GetOp() const { return TypeClassOp; }

const std::string &TypeClass::GetName() const { return name_; }

bool TypeClass::operator==(const Type &other) const {
  return (other.GetOp() == GetOp()) &&
         (name_ == static_cast<const TypeClass &>(other).name_);
};

class TypeOut : public TypeVisitor<void> {
 public:
  TypeOut(std::ostream &out) : out_(out) {}

  virtual void VisitVoid(const TypeVoid t) { out_ << "void"; }

  virtual void VisitInt(const TypeInt t) { out_ << "int"; }

  virtual void VisitBool(const TypeBool t) { out_ << "bool"; }

  virtual void VisitArray(const TypeArray t) {
    this->Visit(t.GetElem());
    out_ << "[]";
  };

  virtual void VisitClass(const TypeClass t) { out_ << t.GetName(); }

 private:
  std::ostream &out_;
};

std::ostream &operator<<(std::ostream &os, const Type &type) {
  TypeOut(os).Visit(type);
  return os;
}
}  // namespace mjc
