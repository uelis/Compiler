package minijava.intermediate.translate;

import minijava.analysis.SymbolTable;
import minijava.intermediate.Label;
import minijava.intermediate.Temp;
import minijava.intermediate.tree.*;
import minijava.syntax.*;

import java.util.LinkedList;
import java.util.List;

import static minijava.intermediate.tree.TreeExpBinOp.Op.*;

class TranslateExp implements ExpVisitor<TreeExp, RuntimeException> {

  private final Translator translator;

  // for better readability
  private TreeExp translate(Exp e) {
    return e.accept(this);
  }

  TranslateExp(Translator translator) {
    this.translator = translator;
  }

  @Override
  public TreeExp visit(ExpTrue e) {
    return new TreeExpConst(1);
  }

  @Override
  public TreeExp visit(ExpFalse e) {
    return new TreeExpConst(0);
  }

  @Override
  public TreeExp visit(ExpThis e) {
    return new TreeExpParam(0);
  }

  @Override
  public TreeExp visit(ExpNewIntArray e) {
    return translator.getRuntime().callNewIntArray(translate(e.getSize()));
  }

  @Override
  public TreeExp visit(ExpNew e) {
    return translator.getRuntime().callNew(e.getClassName());
  }

  @Override
  public TreeExp visit(ExpNeg e) {
    return new TreeExpBinOp(XOR, translate(e.getExp()), new TreeExpConst(1));
  }

  @Override
  public TreeExp visit(ExpBinOp e) {
    TreeExp left = translate(e.getLeft());
    TreeExp right = translate(e.getRight());

    switch (e.getOp()) {
      case DIV:
        return new TreeExpBinOp(DIV, left, right);
      case MINUS:
        return new TreeExpBinOp(MINUS, left, right);
      case PLUS:
        return new TreeExpBinOp(PLUS, left, right);
      case TIMES:
        return new TreeExpBinOp(MUL, left, right);
      case AND:
      case LT:
        TreeExp te = new TreeExpTemp(new Temp());
        Label lTrue = new Label();
        Label lFalse = new Label();
        TreeStm tsCond = e.accept(new TranslateCond(translator, lTrue, lFalse));
        TreeStm stm = new TreeStmSeq(
                new TreeStmMove(te, new TreeExpConst(0)),
                tsCond,
                new TreeStmLabel(lTrue),
                new TreeStmMove(te, new TreeExpConst(1)),
                new TreeStmLabel(lFalse)
        );
        return new TreeExpESeq(stm, te);
    }
    throw new UnsupportedOperationException("Not supported yet.");
  }

  @Override
  public TreeExp visit(ExpArrayGet e) {
    return translator.getRuntime().arrayGet(translate(e.getArray()), translate(e.getIndex()));
  }

  @Override
  public TreeExp visit(ExpArrayLength e) {
    return translator.getRuntime().arrayLength(translate(e.getArray()));
  }

  @Override
  public TreeExp visit(ExpInvoke e) {
    List<TreeExp> args = new LinkedList<>();
    SymbolTable.MethodEntry methodEntry = e.getClassEntry().getMethodEntry(e.getMethod());
    // nonstatic methods get 'this'-pointer as first argument
    TreeExp thisExp = null;
    if (!(methodEntry instanceof SymbolTable.StaticMethodEntry)) {
      thisExp = translate(e.getObj());
      args.add(thisExp); // pointer to object
    }
    for (Exp arg : e.getArgs()) {
      args.add(translate(arg));
    }
    TreeExp methodAddress = translator.getRuntime().methodAddress(thisExp, methodEntry.getDefiningClass().getClassName(), methodEntry.getMethodName());
    return new TreeExpCall(methodAddress, args);
  }

  @Override
  public TreeExp visit(ExpRead e) {
    return translator.getRuntime().callRead();
  }

  @Override
  public TreeExp visit(ExpIntConst e) {
    return new TreeExpConst(e.getValue());
  }

  @Override
  public TreeExp visit(ExpId e) {
    return translator.idAccess(e.getId());
  }
}
