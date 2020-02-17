package minijava.analysis;

import minijava.analysis.SymbolTable.ClassEntry;
import minijava.analysis.SymbolTable.MethodEntry;
import minijava.syntax.*;

import java.util.Iterator;

public class TypeChecking {

  public static void checkPrg(SymbolTable ct, Program p) throws TypeCheckException {

    checkInheritance(ct);

    checkMainClass(ct, p.mainClass);

    for (ClassDeclaration c : p.classes) {
      checkClass(ct, c);
    }
  }

  private static void checkInheritance(SymbolTable symbolTable) throws TypeCheckException {

    for (String className : symbolTable.getClasses()) {
      ClassEntry classEntry = symbolTable.getClassEntry(className);
      for (String m : classEntry.getMethods()) {
        for (ClassEntry c = classEntry.getSuperClass(); c != null; c = c.getSuperClass()) {
          if (c.getMethods().contains(m)) {
            MethodEntry overridee = c.getMethodEntry(m);
            MethodEntry overrider = classEntry.getMethodEntry(m);
            if (overrider.getParams().size() != overridee.getParams().size()) {
              throw new TypeCheckException("Argument count of method "
                      + overrider.getMethodName() + " of class " + overrider.getDefiningClass().getClassName()
                      + " do not match overridden method in class " + overridee.getDefiningClass().getClassName());
            }

            Iterator<String> i1 = overrider.getParams().iterator();
            Iterator<String> i2 = overridee.getParams().iterator();
            while (i1.hasNext()) {
              Type t1 = overrider.getParamType(i1.next());
              Type t2 = overridee.getParamType(i2.next());
              if (!t1.equals(t2)) {
                throw new TypeCheckException("Argument types of method "
                        + overrider.getMethodName() + " of class " + overrider.getDefiningClass().getClassName()
                        + " do not match overridden method in class " + overridee.getDefiningClass().getClassName());
              }
            }
            if (!TypeChecking.subtypeOf(symbolTable, overrider.getReturnType(), overridee.getReturnType())) {
              throw new TypeCheckException("Return type of method "
                      + overrider.getMethodName() + " of class " + overrider.getDefiningClass().getClassName()
                      + " must be subtype of return type of overriden method in class "
                      + overridee.getDefiningClass().getClassName());
            }
          }
        }
      }
    }

  }

  private static void checkMainClass(SymbolTable symbolTable, MainClassDeclaration c)
          throws TypeCheckException {
    SymbolTable.ClassEntry currentClass = symbolTable.getClassEntry(c.getClassName());
    SymbolTable.MethodEntry currentMethod = currentClass.getMethodEntry("main");
    assert (currentMethod != null);
    c.getMainBody().accept(new TypeCheckStm(symbolTable, currentClass, currentMethod));
  }

  private static void checkClass(SymbolTable symbolTable, ClassDeclaration c)
          throws TypeCheckException {

    SymbolTable.ClassEntry currentClass = symbolTable.getClassEntry(c.getClassName());
    assert (currentClass != null);

    for (VariableDeclaration dv : c.getFields()) {
      if (!isWellformedTy(symbolTable, dv.getType())) {
        throw new TypeCheckException("Field " + dv.getName() + " in class "
                + c.getClassName() + " has invalid type " + dv.getType() + ".");
      }
    }

    for (MethodDeclaration m : c.getMethods()) {
      checkMethod(symbolTable, currentClass, m);
    }
  }

  private static void checkMethod(SymbolTable symbolTable,
                                  SymbolTable.ClassEntry currentClass,
                                  MethodDeclaration m) throws TypeCheckException {

    SymbolTable.MethodEntry currentMethod = currentClass.getMethodEntry(m.getMethodName());
    assert (currentMethod != null);

    for (Parameter p : m.getParameters()) {
      if (!isWellformedTy(symbolTable, p.getType())) {
        throw new TypeCheckException("Field " + p.getId() + " in method "
                + m.getMethodName() + " has invalid type " + p.getType() + ".");
      }
    }

    if (!isWellformedTy(symbolTable, m.getReturnType())) {
      throw new TypeCheckException("Return type " + m.getReturnType() + " of method "
              + m.getMethodName() + " is invalid.");
    }

    m.getBody().accept(new TypeCheckStm(symbolTable, currentClass, currentMethod));

    Type returnType = m.getReturnExp().accept(new TypeCheckExp(symbolTable, currentClass, currentMethod));
    if (returnType == null || !subtypeOf(symbolTable, returnType, m.getReturnType())) {
      throw new TypeCheckException("Return type of method " + m.getMethodName()
              + " is " + returnType + ", but should be " + m.getReturnType());
    }
  }

  @SuppressWarnings("BooleanMethodIsAlwaysInverted")
  private static boolean isWellformedTy(SymbolTable symbolTable, Type type) {
    return type.accept(new TypeCheckExp.WellFormedTy(symbolTable));
  }

  private static boolean subtypeOf(SymbolTable table, Type t1, Type t2) {
    if (t1.equals(t2)) {
      return true;
    }

    if (t1 instanceof TypeClass && t2 instanceof TypeClass) {
      SymbolTable.ClassEntry c1 = table.getClassEntry(((TypeClass) t1).getName());
      SymbolTable.ClassEntry c2 = table.getClassEntry(((TypeClass) t2).getName());
      while (c1 != null) {
        if (c1 == c2) {
          return true;
        }
        c1 = c1.getSuperClass();
      }
    }
    return false;
  }

  private static Type getIdType(SymbolTable.ClassEntry currentClass,
                                SymbolTable.MethodEntry currentMethod,
                                String id) throws TypeCheckException {
    Type type = currentMethod.getLocalType(id);
    if (type == null) {
      type = currentMethod.getParamType(id);
      if (type == null) {
        SymbolTable.ClassEntry c = currentClass;
        while (type == null && c != null) {
          type = c.getFieldType(id);
          c = c.getSuperClass();
        }
        if (type == null) {
          throw new TypeCheckException("Undeclared identifier " + id + " in method " + currentMethod.getMethodName() + " of class " + currentClass.getClassName());
        }
        // This should never trigger now, as the main class doesn't have fields.
        if (currentMethod instanceof SymbolTable.StaticMethodEntry) {
          throw new TypeCheckException("Static methods may only access "
                  + "static fields");
        }
      }
    }
    return type;
  }

  static private class TypeCheckStm implements StmVisitor<Void, TypeCheckException> {

    final SymbolTable symbolTable;
    final SymbolTable.ClassEntry currentClass;
    final SymbolTable.MethodEntry currentMethod;

    TypeCheckStm(SymbolTable symbolTable, SymbolTable.ClassEntry currentClass,
                 SymbolTable.MethodEntry currentMethod) {
      this.symbolTable = symbolTable;
      this.currentClass = currentClass;
      this.currentMethod = currentMethod;
    }

    private void throwException(Stm s, String msg) throws TypeCheckException {
      throw new TypeCheckException("Type error in statement " + s.prettyPrint()
              + " in method " + currentMethod.getMethodName()
              + " of class " + currentClass.getClassName() + ":\n" + msg);
    }

    private Type checkExp(Exp e) throws TypeCheckException {
      return e.accept(new TypeCheckExp(symbolTable, currentClass, currentMethod));
    }

    private Type getIdType(String id) throws TypeCheckException {
      return TypeChecking.getIdType(currentClass, currentMethod, id);
    }

    @Override
    public Void visit(StmSeq stmList) throws TypeCheckException {
      for (Stm s : stmList.getStmList()) {
        s.accept(this);
      }
      return null;
    }

    @Override
    public Void visit(StmIf s) throws TypeCheckException {

      Type condType = checkExp(s.getCond());

      if (!(condType instanceof TypeBoolean)) {
        throwException(s, "Condition must be of boolean type, but has type " + condType);
      }
      s.getFalseBranch().accept(this);
      s.getTrueBranch().accept(this);
      return null;
    }

    @Override
    public Void visit(StmWhile s) throws TypeCheckException {
      Type condType = checkExp(s.getCond());

      if (!(condType instanceof TypeBoolean)) {
        throwException(s, "Condition must be of boolean type, but has type " + condType);
      }

      s.getBody().accept(this);
      return null;
    }

    @Override
    public Void visit(StmPrintln s) throws TypeCheckException {
      Type argType = checkExp(s.getArg());
      if (!(argType instanceof TypeInt)) {
        throwException(s, "Argument for println must be of integer type, but has type " + argType);
      }
      return null;
    }

    @Override
    public Void visit(StmWrite s) throws TypeCheckException {
      Type argType = checkExp(s.getExp());
      if (!(argType instanceof TypeInt)) {
        throwException(s, "Argument for print((char)...) must be of integer type, but has type " + argType);
      }
      return null;
    }

    @Override
    public Void visit(StmAssign s) throws TypeCheckException {

      Type rhsType = checkExp(s.getExp());
      Type lhsType = getIdType(s.getId());

      if (!subtypeOf(symbolTable, rhsType, lhsType)) {
        if (subtypeOf(symbolTable, lhsType, rhsType)) {
          System.out.println("Warning: Implicit cast from " + rhsType + " to " + lhsType);

        } else {
          throwException(s, "Type mismatch in assignment: lhs has type " + lhsType + ", rhs has type " + rhsType);
        }
      }
      return null;
    }

    @Override
    public Void visit(StmArrayAssign s) throws TypeCheckException {

      Type indexType = checkExp(s.getIndexExp());

      if (!(indexType instanceof TypeInt)) {
        throwException(s, "Array index must be of integer type, but has type " + indexType);
      }

      Type arrType = getIdType(s.getId());

      if (!(arrType instanceof TypeArray)) {
        throwException(s, "Accessed expression must be an array, but has type " + arrType);
      }

      Type elemType = ((TypeArray) arrType).getEntryType();

      Type rhsType = checkExp(s.getExp());

      if (!subtypeOf(symbolTable, rhsType, elemType)) {
        throwException(s, "Type mismatch in array assignment: rhs has type " + rhsType + ", but elements have type " + elemType);
      }
      return null;
    }
  }

  static private class TypeCheckExp implements ExpVisitor<Type, TypeCheckException> {

    final SymbolTable classTable;
    final SymbolTable.ClassEntry currentClass;
    final SymbolTable.MethodEntry currentMethod;

    TypeCheckExp(SymbolTable classTable, SymbolTable.ClassEntry currentClass,
                 SymbolTable.MethodEntry currentMethod) {
      this.classTable = classTable;
      this.currentClass = currentClass;
      this.currentMethod = currentMethod;
    }

    private Type checkExp(Exp e) throws TypeCheckException {
      return e.accept(this);
    }

    private Type getIdType(String id) throws TypeCheckException {
      return TypeChecking.getIdType(currentClass, currentMethod, id);
    }

    private void throwException(Exp e, String msg) throws TypeCheckException {
      throw new TypeCheckException("Type error in expression " + e.prettyPrint()
              + ", in method " + currentMethod.getMethodName()
              + " of class " + currentClass.getClassName() + ":\n" + msg);
    }

    @Override
    public Type visit(ExpTrue e) {
      return new TypeBoolean();
    }

    @Override
    public Type visit(ExpFalse e) {
      return new TypeBoolean();
    }

    @Override
    public Type visit(ExpThis e) throws TypeCheckException {
      if (currentMethod instanceof SymbolTable.StaticMethodEntry) {
        throwException(e, "Cannot access 'this' in static method");
      }
      return new TypeClass(currentClass.getClassName());
    }

    @Override
    public Type visit(ExpNewIntArray e) throws TypeCheckException {

      Type sizeType = checkExp(e.getSize());

      if (!(sizeType instanceof TypeInt)) {
        throwException(e, "Array size must be of integer type, but has type " + sizeType);
      }
      return new TypeArray(new TypeInt());
    }

    @Override
    public Type visit(ExpNew e) {
      return new TypeClass(e.getClassName());
    }

    @Override
    public Type visit(ExpNeg e) throws TypeCheckException {

      Type bodyType = checkExp(e.getExp());

      if (!(bodyType instanceof TypeBoolean)) {
        throwException(e, "Negation can only be used with booleans, but expression has type " + bodyType);
      }
      return new TypeBoolean();
    }

    @Override
    public Type visit(ExpBinOp e) throws TypeCheckException {

      Type lType = checkExp(e.getLeft());
      Type rType = checkExp(e.getRight());
      Type expectedType = (e.getOp() == ExpBinOp.Op.AND) ? new TypeBoolean() : new TypeInt();
      if (!lType.equals(expectedType) || !rType.equals(expectedType)) {
        throwException(e, "Both expressions must have type " + expectedType + ",\n"
                + "but left operand has type " + lType + " and right operand has type " + rType);
      }

      switch (e.getOp()) {
        case LT:
        case AND:
          return new TypeBoolean();
        case PLUS:
        case MINUS:
        case TIMES:
        case DIV:
          return new TypeInt();
      }
      assert (false);
      return null;
    }

    @Override
    public Type visit(ExpArrayGet e) throws TypeCheckException {

      Type arrType = checkExp(e.getArray());
      Type indexType = checkExp(e.getIndex());

      if (!(indexType instanceof TypeInt)) {
        throwException(e, "Array index must be of integer type, but has type " + indexType);
      }

      if (!(arrType instanceof TypeArray)) {
        throwException(e, "Accessed expression must be an array, but has type " + arrType);
      }

      return ((TypeArray) arrType).getEntryType();
    }

    @Override
    public Type visit(ExpArrayLength e) throws TypeCheckException {
      Type arrType = checkExp(e.getArray());
      if (!(arrType instanceof TypeArray)) {
        throwException(e, "Accessed expression must be an array, but has type " + arrType);
      }
      return new TypeInt();
    }

    @Override
    public Type visit(ExpInvoke e) throws TypeCheckException {
      Type objType = checkExp(e.getObj());

      if (!(objType instanceof TypeClass)) {
        throwException(e, "Expression " + e.getObj().prettyPrint() + " must be an object, but has type " + objType);
      }

      String className = ((TypeClass) objType).getName();

      SymbolTable.ClassEntry classEntry = classTable.getClassEntry(className);
      if (classEntry == null) {
        throwException(e, "Expression " + e.getObj().prettyPrint() + " is of undeclared class " + className);
      }

      SymbolTable.MethodEntry methodEntry = classEntry.getMethodEntry(e.getMethod());
      while (methodEntry == null && classEntry != null) {
        classEntry = classEntry.getSuperClass();
        if (classEntry != null) {
          methodEntry = classEntry.getMethodEntry(e.getMethod());
        }
      }

      if (methodEntry == null) {
        throwException(e, "TreeFunction '" + e.getMethod() + "' not declared.");
      }

      if (methodEntry.throwsIOException() && !currentMethod.throwsIOException()) {
        throwException(e, "Expression " + e + " may throw IOException, which must be declared.");
      }

      // annotate syntax tree with class entry
      e.setClassEntry(classEntry);

      // Check argument types
      Iterator<String> params = methodEntry.getParams().iterator();
      Iterator<Exp> args = e.getArgs().listIterator();

      while (params.hasNext() && args.hasNext()) {
        Type type = methodEntry.getParamType(params.next());
        Type argType = checkExp(args.next());
        if (type == null || !subtypeOf(classTable, argType, type)) {
          throwException(e, "Argument type mismatch: expected " + type + ", but found " + argType);
        }
      }
      if (params.hasNext() || args.hasNext()) {
        throwException(e, "Wrong number of arguments");
      }

      return methodEntry.getReturnType();
    }

    @Override
    public Type visit(ExpRead e) throws TypeCheckException {
      if (!currentMethod.throwsIOException()) {
        throwException(e, "Expression " + e + " may throw IOException, which must be declared.");
      }
      return new TypeInt();
    }

    @Override
    public Type visit(ExpIntConst e) {
      return new TypeInt();
    }

    @Override
    public Type visit(ExpId e) throws TypeCheckException {
      return getIdType(e.getId());
    }

    static private class WellFormedTy implements TypeVisitor<Boolean> {

      final SymbolTable symbolTable;

      WellFormedTy(SymbolTable symbolTable) {
        this.symbolTable = symbolTable;
      }

      @Override
      public Boolean visit(TypeVoid t) {
        return true;
      }

      @Override
      public Boolean visit(TypeBoolean t) {
        return true;
      }

      @Override
      public Boolean visit(TypeInt t) {
        return true;
      }

      @Override
      public Boolean visit(TypeClass t) {
        return symbolTable.getClassEntry(t.getName()) != null;
      }

      @Override
      public Boolean visit(TypeArray t) {
        return t.getEntryType().accept(this);
      }
    }
  }
}
