package minijava.intermediate.translate;

import minijava.intermediate.Label;
import minijava.intermediate.tree.*;
import minijava.syntax.*;

class TranslateCond implements ExpVisitor<TreeStm, RuntimeException> {

  private final Translator translator;
  private final Label lTrue;
  private final Label lFalse;

  public TranslateCond(Translator translator, Label lTrue, Label lFalse) {
    this.translator = translator;
    this.lTrue = lTrue;
    this.lFalse = lFalse;
  }

  @Override
  public TreeStm visit(ExpTrue e) {
    return new TreeStmJump(lTrue);
  }

  @Override
  public TreeStm visit(ExpFalse e) {
    return new TreeStmJump(lFalse);
  }

  @Override
  public TreeStm visit(ExpThis e) {
    throw new Error("Internal Error: Not a boolean!");
  }

  @Override
  public TreeStm visit(ExpNewIntArray e) {
    throw new Error("Internal Error: Not a boolean!");
  }

  @Override
  public TreeStm visit(ExpNew e) {
    throw new Error("Internal Error: Not a boolean!");
  }

  @Override
  public TreeStm visit(ExpNeg e) {
    return e.getExp().accept(new TranslateCond(translator, lFalse, lTrue));
  }

  @Override
  public TreeStm visit(ExpBinOp e) {
    switch (e.getOp()) {
      case AND: {
        Label l = new Label();
        TreeStm left = e.getLeft().accept(new TranslateCond(translator, l, lFalse));
        TreeStm right = e.getRight().accept(new TranslateCond(translator, lTrue, lFalse));
        return new TreeStmSeq(
                left,
                new TreeStmLabel(l),
                right
        );
      }
      case LT: {
        TreeExp left = e.getLeft().accept(new TranslateExp(translator));
        TreeExp right = e.getRight().accept(new TranslateExp(translator));
        return new TreeStmCJump(TreeStmCJump.Rel.GE, left, right, lFalse, lTrue);
      }
    }
    throw new Error("Internal Error: Not a boolean!");
  }

  @Override
  public TreeStm visit(ExpArrayGet e) {
    throw new Error("Internal Error: Not a boolean!");
  }

  @Override
  public TreeStm visit(ExpArrayLength e) {
    throw new Error("Internal Error: Not a boolean!");
  }

  @Override
  public TreeStm visit(ExpInvoke e) {
    TreeExp exp = e.accept(new TranslateExp(translator));
    return new TreeStmCJump(TreeStmCJump.Rel.NE, exp, new TreeExpConst(0), lTrue, lFalse);
  }

  @Override
  public TreeStm visit(ExpRead e) {
    TreeExp exp = e.accept(new TranslateExp(translator));
    return new TreeStmCJump(TreeStmCJump.Rel.NE, exp, new TreeExpConst(0), lTrue, lFalse);
  }

  @Override
  public TreeStm visit(ExpIntConst e) {
    throw new Error("Internal Error: Not a boolean!");
  }

  @Override
  public TreeStm visit(ExpId e) {
    TreeExp exp = e.accept(new TranslateExp(translator));
    return new TreeStmCJump(TreeStmCJump.Rel.NE, exp, new TreeExpConst(0), lTrue, lFalse);
  }
}
