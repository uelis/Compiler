package minijava.intermediate.translate;

import minijava.intermediate.Label;
import minijava.intermediate.Temp;
import minijava.intermediate.tree.*;
import minijava.syntax.*;

import java.util.ArrayList;
import java.util.List;

class TranslateStm implements StmVisitor<TreeStm, RuntimeException> {

  private final Translator translator;

  TranslateStm(Translator translator) {
    this.translator = translator;
  }

  // for better readability
  private TreeStm translate(Stm s) {
    TreeStm t = s.accept(this);
    assert (t != null);
    return t;
  }

  // for better readability
  private TreeExp translate(Exp e) {
    TreeExp t = e.accept(new TranslateExp(translator));
    assert (t != null);
    return t;
  }

  @Override
  public TreeStm visit(StmSeq slist) {
    List<TreeStm> ts = new ArrayList<>();
    for (Stm s : slist.getStmList()) {
      TreeStm t = translate(s);
      ts.add(t);
    }
    return new TreeStmSeq(ts);
  }

  @Override
  public TreeStm visit(StmIf s) {
    TreeStm tsTrue = translate(s.getTrueBranch());
    TreeStm tsFalse = translate(s.getFalseBranch());
    Label lTrue = new Label();
    Label lFalse = new Label();
    Label lExit = new Label();
    TreeStm tsCond = s.getCond().accept(new TranslateCond(translator, lTrue, lFalse));

    return new TreeStmSeq(
            tsCond,
            new TreeStmLabel(lTrue),
            tsTrue,
            new TreeStmJump(lExit),
            new TreeStmLabel(lFalse),
            tsFalse,
            new TreeStmLabel(lExit)
    );
  }

  @Override
  public TreeStm visit(StmWhile s) {
    TreeStm tsBody = translate(s.getBody());
    Label lTop = new Label();
    Label lTrue = new Label();
    Label lFalse = new Label();
    TreeStm tsCond = s.getCond().accept(new TranslateCond(translator, lTrue, lFalse));

    return new TreeStmSeq(
            new TreeStmLabel(lTop),
            tsCond,
            new TreeStmLabel(lTrue),
            tsBody,
            new TreeStmJump(lTop),
            new TreeStmLabel(lFalse)
    );
  }

  @Override
  public TreeStm visit(StmPrintln s) {
    return new TreeStmMove(new TreeExpTemp(new Temp()),
            translator.getRuntime().callPrintln(translate(s.getArg())));
  }

  @Override
  public TreeStm visit(StmWrite s) {
    return new TreeStmMove(new TreeExpTemp(new Temp()),
            translator.getRuntime().callWrite(translate(s.getExp())));
  }

  @Override
  public TreeStm visit(StmAssign s) {
    return new TreeStmMove(translator.idAccess(s.getId()), translate(s.getExp()));
  }

  @Override
  public TreeStm visit(StmArrayAssign s) {
    return translator.getRuntime().arrayPut(translator.idAccess(s.getId()), translate(s.getIndexExp()), translate(s.getExp()));
  }
}
