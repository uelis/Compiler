//
// Representation of MiniJava types
//

#ifndef MJC_MINIJAVA_TYPE_H
#define MJC_MINIJAVA_TYPE_H

#include <cassert>
#include <iostream>
#include <memory>
#include <optional>

#include "minijava/location.hh"

namespace mjc {

// Ast for MiniJava types
class Type {
public:
  enum Op { TypeVoidOp, TypeIntOp, TypeBoolOp, TypeArrayOp, TypeClassOp };

  virtual ~Type() {}

  virtual const Op GetOp() const = 0;

  void SetLocation(location l);
  std::optional<location> GetLocation() const;

  virtual bool operator==(const Type &other) const;
  bool operator!=(const Type &other) const;

private:
  std::optional<location> location_;
};

std::ostream &operator<<(std::ostream &os, const Type &type);

class TypeVoid : public Type {
public:
  explicit TypeVoid();

  virtual const Op GetOp() const;
  virtual bool operator==(const Type &other) const;
};

class TypeInt : public Type {
public:
  explicit TypeInt();

  virtual const Op GetOp() const;
  virtual bool operator==(const Type &other) const;
};

class TypeBool : public Type {
public:
  explicit TypeBool();

  virtual const Op GetOp() const;
  virtual bool operator==(const Type &other) const;
};

class TypeArray : public Type {
public:
  explicit TypeArray(std::shared_ptr<Type> elem);

  virtual const Op GetOp() const;
  const Type &GetElem() const;
  virtual bool operator==(const Type &other) const;

private:
  const std::shared_ptr<Type> elem_;
};

class TypeClass : public Type {
public:
  explicit TypeClass(std::string name);

  virtual const Op GetOp() const;
  const std::string &GetName() const;
  virtual bool operator==(const Type &other) const;

private:
  const std::string name_;
};

template <typename RetTy> class TypeVisitor {
public:
  virtual RetTy VisitVoid(const TypeVoid t) = 0;
  virtual RetTy VisitInt(const TypeInt t) = 0;
  virtual RetTy VisitBool(const TypeBool t) = 0;
  virtual RetTy VisitArray(const TypeArray t) = 0;
  virtual RetTy VisitClass(const TypeClass t) = 0;

  RetTy Visit(const Type &t) {
    switch (t.GetOp()) {
    case Type::TypeVoidOp:
      return VisitVoid(static_cast<const TypeVoid &>(t));
    case Type::TypeIntOp:
      return VisitInt(static_cast<const TypeInt &>(t));
    case Type::TypeBoolOp:
      return VisitBool(static_cast<const TypeBool &>(t));
    case Type::TypeArrayOp:
      return VisitArray(static_cast<const TypeArray &>(t));
    case Type::TypeClassOp:
      return VisitClass(static_cast<const TypeClass &>(t));
    }
    assert(false);
  }
};
} // namespace mjc

#endif
