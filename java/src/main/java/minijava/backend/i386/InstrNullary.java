package minijava.backend.i386;

import minijava.intermediate.Label;
import minijava.intermediate.Temp;
import minijava.util.Pair;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.function.Function;

@SuppressWarnings("SameParameterValue")
final class InstrNullary implements I386Instruction {

  enum Kind {
    RET, LEAVE, NOP
  }

  private final Kind kind;

  InstrNullary(Kind kind) {
    this.kind = kind;
  }

  @Override
  public List<Temp> use() {
    switch (kind) {
      case RET: {
        List<Temp> use = new ArrayList<>(Registers.calleeSave);
        use.add(Registers.eax);
        return use;
      }
      case LEAVE:
      case NOP:
        return Collections.emptyList();
      default:
        assert false;
        return null;
    }
  }

  @Override
  public List<Temp> def() {
    return Collections.emptyList();
  }

  @Override
  public List<Label> jumps() {
    return Collections.emptyList();
  }

  @Override
  public boolean isFallThrough() {
    return (kind != Kind.RET);
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
            .append('\n');
  }

  @Override
  public void rename(Function<Temp, Temp> sigma) {
  }
}
