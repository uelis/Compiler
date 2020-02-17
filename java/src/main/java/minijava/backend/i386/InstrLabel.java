package minijava.backend.i386;

import minijava.intermediate.Label;
import minijava.intermediate.Temp;
import minijava.util.Pair;

import java.util.Collections;
import java.util.List;
import java.util.function.Function;

final class InstrLabel implements I386Instruction {

  private final Label label;

  InstrLabel(Label label) {
    this.label = label;
  }

  @Override
  public List<Temp> use() {
    return Collections.emptyList();
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
    return true;
  }

  @Override
  public Pair<Temp, Temp> isMoveBetweenTemps() {
    return null;
  }

  @Override
  public Label isLabel() {
    return label;
  }

  @Override
  public void render(StringBuilder s) {
    s.append(label);
    s.append(":\n");
  }

  @Override
  public void rename(Function<Temp, Temp> sigma) {
  }
}
