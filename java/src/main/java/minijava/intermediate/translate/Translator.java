package minijava.intermediate.translate;

import minijava.analysis.SymbolTable;
import minijava.backend.Platform;
import minijava.intermediate.Label;
import minijava.intermediate.Temp;
import minijava.intermediate.tree.*;
import minijava.syntax.*;

import java.util.*;

public class Translator {

  private final Runtime runtime;
  private final SymbolTable symbolTable;
  private SymbolTable.ClassEntry currentClass = null;
  private SymbolTable.MethodEntry currentMethod = null;
  private final Map<String, Temp> names = new HashMap<>();

  public Translator(Platform platform, SymbolTable st) {
    this.symbolTable = st;
    this.runtime = new Runtime(platform, st);
  }

  public TreePrg translatePrg(Program p) {
    List<TreeFunction> methods = new LinkedList<>();

    methods.add(translateMainClass(p.mainClass));

    for (ClassDeclaration dc : p.classes) {
      methods.addAll(translateClass(dc));
    }
    return new TreePrg(methods);
  }

  Runtime getRuntime() {
    return runtime;
  }

  private TreeFunction translateMainClass(MainClassDeclaration dm) {
    currentClass = symbolTable.getClassEntry(dm.getClassName());

    return translateMeth("main", dm.getMainBody(), new ExpIntConst(0));
  }

  private List<TreeFunction> translateClass(ClassDeclaration dc) {
    currentClass = symbolTable.getClassEntry(dc.getClassName());

    List<TreeFunction> methods = new LinkedList<>();
    for (MethodDeclaration dm : dc.getMethods()) {
      methods.add(translateMeth(dm.getMethodName(), dm.getBody(), dm.getReturnExp()));
    }
    return methods;
  }

  private TreeFunction translateMeth(String methodName, Stm methodBody, Exp returnExp) {

    assert (currentClass != null);
    currentMethod = currentClass.getMethodEntry(methodName);
    assert (currentMethod != null);
    names.clear();

    Label l = new Label();
    Temp r = new Temp();
    TreeStm raiseBlock = runtime.newRaiseBlock();
    List<TreeStm> translatedBody = Arrays.asList(
            methodBody.accept(new TranslateStm(this)),
            new TreeStmJump(l),
            raiseBlock,
            new TreeStmLabel(l),
            new TreeStmMove(new TreeExpTemp(r), returnExp.accept(
                    new TranslateExp(this)))
    );
    return new TreeFunction(
            runtime.methodLabel(currentClass.getClassName(), methodName),
            1 + currentMethod.getParams().size(),
            translatedBody,
            r
    );
  }

  TreeExp idAccess(String name) {
    if (currentMethod.getLocals().contains(name)) {
      Temp t = names.computeIfAbsent(name, (x) -> new Temp());
      return new TreeExpTemp(t);
    } else if (currentMethod.getParams().contains(name)) {
      return new TreeExpParam(1 /* this */ + currentMethod.getParams().indexOf(name));
    } else {
      TreeExp access = runtime.fieldAddress(new TreeExpParam(0),
              currentClass.getClassName(), name);
      if (access == null) {
        throw new RuntimeException("Internal error: Cannot find id + " + name);
      }
      return access;
    }
  }
}
