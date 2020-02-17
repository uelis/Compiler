package minijava.backend.i386;

import minijava.intermediate.Label;
import minijava.intermediate.Temp;
import minijava.util.Pair;

import java.util.Collections;
import java.util.List;
import java.util.function.Function;

@SuppressWarnings("SameParameterValue")
final class InstrJump implements I386Instruction {

  enum Kind {
    JMP, J, CALL
  }

  @SuppressWarnings("unused")
  enum Cond {
    E, NE, L, LE, G, GE, Z
  }

  private final Kind kind;
  private final Label label;
  private Operand dest;
  private final Cond cond;

  InstrJump(Kind kind, Label label) {
    this(kind, label, null, null);
  }

  InstrJump(Kind kind, Operand dest) {
    this(kind, null, dest, null);
  }

  InstrJump(Cond cond, Label label) {
    this(Kind.J, label, null, cond);
  }

  @SuppressWarnings("WeakerAccess")
  InstrJump(Kind kind, Label label, Operand dest, Cond cond) {
    assert (kind != Kind.J || cond != null) : "J needs condition argument";
    assert (kind == Kind.CALL || label != null) : "J and JMP need label as destination";
    assert (dest == null || dest instanceof Operand.Reg) : "dynamic destination of CALL must be Reg";
    this.kind = kind;
    this.label = label;
    this.dest = dest;
    this.cond = cond;
  }

  @Override
  public List<Temp> use() {
    if (dest != null) {
      return dest.use();
    }
    return Collections.emptyList();
  }

  @Override
  public List<Temp> def() {
    if (kind == Kind.CALL) {
      return Registers.callerSave;
    } else {
      return Collections.emptyList();
    }
  }

  @Override
  public List<Label> jumps() {
    switch (kind) {
      case JMP:
      case J:
        return Collections.singletonList(label);
      case CALL:
        return Collections.emptyList();
      default:
        assert false;
        return null;
    }
  }

  @Override
  public boolean isFallThrough() {
    return (kind == Kind.J || kind == Kind.CALL);
  }

  @Override
  public Pair<Temp, Temp> isMoveBetweenTemps() {
    return null;
  }

  @Override
  public Label isLabel() {
    return null;
  }

  @Override
  public void render(StringBuilder s) {
    s.append('\t');
    if (kind == Kind.J) {
      s.append(kind)
              .append(cond);
    } else {
      s.append(kind);
    }
    s.append(' ')
            .append((label != null) ? label : dest)
            .append('\n');
  }

  @Override
  public void rename(Function<Temp, Temp> sigma) {
    if (dest != null) {
      dest = dest.rename(sigma);
    }
  }
}
