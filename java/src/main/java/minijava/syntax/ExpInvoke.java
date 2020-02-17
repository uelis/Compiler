package minijava.syntax;

import minijava.analysis.SymbolTable;

import java.util.Collections;
import java.util.List;

/**
 * Represents a method call {@code obj.method(args)}.
 */
public class ExpInvoke extends Exp {

  private final Exp obj;
  private final String method;
  private final List<Exp> args;
  private SymbolTable.ClassEntry classEntry = null;
  // Class entry in the symbol table for the *static* type of obj.
  // This field will be filled in during type checking.

  public ExpInvoke(Exp obj, String method, List<Exp> args) {
    this.obj = obj;
    this.method = method;
    this.args = args;
  }

  @Override
  public <A, T extends Throwable> A accept(ExpVisitor<A, T> v) throws T {
    return v.visit(this);
  }

  public Exp getObj() {
    return obj;
  }

  public String getMethod() {
    return method;
  }

  public List<Exp> getArgs() {
    return Collections.unmodifiableList(args);
  }

  public SymbolTable.ClassEntry getClassEntry() {
    return classEntry;
  }

  public void setClassEntry(SymbolTable.ClassEntry classEntry) {
    this.classEntry = classEntry;
  }
}
