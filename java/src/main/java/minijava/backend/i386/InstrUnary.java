package minijava.backend.i386;

import minijava.backend.i386.Operand.Reg;
import minijava.intermediate.Label;
import minijava.intermediate.Temp;
import minijava.util.Pair;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.function.Function;

final class InstrUnary implements I386Instruction {

  enum Kind {

    PUSH, POP, NEG, NOT, INC, DEC, IDIV, IMUL
  }

  private Operand op;
  private final Kind kind;

  InstrUnary(Kind kind, Operand op) {
    assert (!(kind == Kind.POP || kind == Kind.NEG || kind == Kind.NOT
            || kind == Kind.INC || kind == Kind.DEC || kind == Kind.IDIV) || !(op instanceof Operand.Imm));
    this.op = op;
    this.kind = kind;
  }

  @Override
  public List<Temp> use() {
    switch (kind) {
      case POP:
        return Collections.emptyList();
      case PUSH:
      case NEG:
      case NOT:
      case INC:
      case DEC:
        return op.use();
      case IDIV: {
        List<Temp> use = new ArrayList<>(op.use());
        use.add(Registers.eax);
        use.add(Registers.edx);
        return use;
      }
      case IMUL: {
        List<Temp> use = new ArrayList<>(op.use());
        use.add(Registers.eax);
        return use;
      }
      default:
        assert (false);
        return null;
    }
  }

  @Override
  public List<Temp> def() {
    switch (kind) {
      case PUSH:
        return Collections.emptyList();
      case POP:
      case NEG:
      case NOT:
      case INC:
      case DEC:
        if (op instanceof Operand.Reg) {
          return Collections.singletonList(((Reg) op).reg);
        } else {
          return Collections.emptyList();
        }
      case IDIV: {
        List<Temp> def = new ArrayList<>(2);
        def.add(Registers.eax);
        def.add(Registers.edx);
        return def;
      }
      case IMUL: {
        List<Temp> def = new ArrayList<>(2);
        def.add(Registers.eax);
        def.add(Registers.edx);
        return def;
      }
      default:
        assert (false);
        return null;
    }
  }

  @Override
  public List<Label> jumps() {
    return Collections.emptyList();
  }

  @Override
  public boolean isFallThrough() {
    return true;
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
    s.append('\t')
            .append(kind)
            .append(' ');
    op.render(s);
    s.append('\n');
  }

  @Override
  public void rename(Function<Temp, Temp> sigma) {
    op = op.rename(sigma);
  }
}
