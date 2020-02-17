package minijava.backend.i386;

import minijava.intermediate.Label;
import minijava.intermediate.Temp;
import minijava.util.Pair;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.function.Function;

final class InstrBinary implements I386Instruction {

  @SuppressWarnings("unused")
  enum Kind {
    MOV, ADD, SUB, SHL, SHR, SAL, SAR, AND, OR, XOR, TEST, CMP, LEA, IMUL
  }

  private Operand src;
  private Operand dst;
  private final Kind kind;

  InstrBinary(Kind kind, Operand dst, Operand src) {
    assert (kind != null && src != null && dst != null);
    assert (!((src instanceof Operand.Mem) && (dst instanceof Operand.Mem)));
    assert (kind != Kind.LEA || (((src instanceof Operand.Mem) || (src instanceof Operand.Sym)) && (dst instanceof Operand.Reg)));
    this.src = src;
    this.dst = dst;
    this.kind = kind;
  }

  @Override
  public List<Temp> use() {
    // XOR r, r does not use r
    if (kind == Kind.XOR && src instanceof Operand.Reg && dst instanceof Operand.Reg) {
      Operand.Reg srcReg = (Operand.Reg) src;
      Operand.Reg dstReg = (Operand.Reg) dst;
      if (srcReg.reg.equals(dstReg.reg)) {
        return Collections.emptyList();
      }
    }
    if ((kind == Kind.MOV || kind == Kind.LEA) && (dst instanceof Operand.Reg)) {
      return src.use();
    }
    List<Temp> use = new ArrayList<>(src.use());
    use.removeAll(dst.use()); // avoid duplicates
    use.addAll(dst.use());
    return use;
  }

  @Override
  public List<Temp> def() {
    if (kind == Kind.CMP || kind == Kind.TEST) {
      return Collections.emptyList();
    }
    if (dst instanceof Operand.Reg) {
      return Collections.singletonList(((Operand.Reg) dst).reg);
    } else {
      return Collections.emptyList();
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
    if (kind == Kind.MOV && (dst instanceof Operand.Reg) && (src instanceof Operand.Reg)) {
      Operand.Reg srcReg = (Operand.Reg) src;
      Operand.Reg dstReg = (Operand.Reg) dst;
      return new Pair<>(dstReg.reg, srcReg.reg);
    } else {
      return null;
    }
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
    dst.render(s);
    s.append(", ");
    src.render(s);
    s.append('\n');
  }

  @Override
  public void rename(Function<Temp, Temp> sigma) {
    dst = dst.rename(sigma);
    src = src.rename(sigma);
  }
}
