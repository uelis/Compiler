//
// Representation of the abstract syntax of MiniJava programs.
//
// The syntax of expressions, statements and types is represened by the
// composites defined in exp.h, stm.h and type.h respectively.
//
// All fields in the structs in this file are non-null.
//

#ifndef MJC_MINIJAVA_AST_H
#define MJC_MINIJAVA_AST_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "minijava/exp.h"
#include "minijava/location.hh"
#include "minijava/stm.h"
#include "minijava/type.h"
#include "util/ordered_map.h"

namespace mjc {

// Identifiers are strings
using Ident = std::string;

// Represents a variable declaration, as used to declare local variables
// and parameters
struct VarDecl {
  Ident var_name;
  std::shared_ptr<Type> var_type;
  location source_location;
};

// Represents one MiniJava method
struct MethodDecl {
  Ident method_name;
  std::shared_ptr<Type> return_type;
  std::vector<VarDecl> parameters;
  bool throws_io_exception;
  std::vector<VarDecl> locals;
  std::shared_ptr<Stm> body;
  std::shared_ptr<Exp> return_exp;
  location source_location;
};

// Represents a class of a MiniJava program
struct ClassDecl {
  Ident class_name;
  std::vector<VarDecl> fields;
  std::vector<MethodDecl> methods;
  location source_location;
};

// Represents the class with the main function in a MiniJava program
struct MainClassDecl {
  Ident class_name;
  bool main_throws_io_exception;
  std::shared_ptr<Stm> main_body;
  location source_location;
};

// Represents a whole MiniJava program
struct Program {
  MainClassDecl main_class;
  std::vector<ClassDecl> classes;
};

}  // namespace mjc

#endif
