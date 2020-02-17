package minijava.backend.i386;

import minijava.intermediate.Temp;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.function.Function;

abstract class Operand {

  final static class Imm extends Operand {

    final int imm;

    Imm(Integer imm) {
      assert (imm != null);
      this.imm = imm;
    }

    @Override
    public Operand rename(Function<Temp, Temp> sigma) {
      return this;
    }

    @Override
    void render(StringBuilder s) {
      s.append(imm);
    }
  }

  final static class Reg extends Operand {

    final Temp reg;

    Reg(Temp reg) {
      assert (reg != null);
      this.reg = reg;
    }

    @Override
    List<Temp> use() {
      return Collections.singletonList(reg);
    }

    @Override
    public Operand rename(Function<Temp, Temp> sigma) {
      return new Reg(sigma.apply(reg));
    }

    @Override
    void render(StringBuilder s) {
      s.append(Registers.nameOf(reg));
    }
  }

  final static class Mem extends Operand {

    final Temp base;  // maybe null
    final Integer scale; // null or 1, 2, 4 or 8;
    final Temp index;  // maybe null
    final int displacement;

    Mem(Temp base, Integer scale, Temp index, int displacement) {
      assert (scale == null || (scale == 1 || scale == 2 || scale == 4 || scale == 8));
      this.base = base;
      this.scale = scale;
      this.index = index;
      this.displacement = displacement;
    }

    Mem(Temp base) {
      this(base, null, null, 0);
    }

    @Override
    void render(StringBuilder s) {
      s.append("DWORD PTR [");
      if (base != null) {
        s.append(Registers.nameOf(base));
        if (scale != null) {
          s.append('+');
        }
      }
      if (scale != null) {
        s.append(Registers.nameOf(index));
        if (scale > 1) {
          s.append('*');
          s.append(scale);
        }
      }
      if (displacement != 0) {
        s.append('+');
        s.append(displacement);
      }
      s.append(']');
    }

    @Override
    List<Temp> use() {
      List<Temp> use = new ArrayList<>(2);
      if (base != null) {
        use.add(base);
      }
      if (index != null) {
        use.add(index);
      }
      return use;
    }

    @Override
    public Operand rename(Function<Temp, Temp> sigma) {
      return new Mem(base != null ? sigma.apply(base) : null, scale,
              index != null ? sigma.apply(index) : null, displacement);
    }
  }

  final static class Sym extends Operand {

    final String symbol;

    Sym(String symbol) {
      assert (symbol != null);
      this.symbol = symbol;
    }

    @Override
    public Operand rename(Function<Temp, Temp> sigma) {
      return this;
    }

    @Override
    void render(StringBuilder s) {
      s.append(symbol);
    }
  }

  final static class FrameSize extends Operand {

    final I386Function function;

    FrameSize(I386Function function) {
      assert (function != null);
      this.function = function;
    }

    @Override
    public Operand rename(Function<Temp, Temp> sigma) {
      return this;
    }

    @Override
    void render(StringBuilder s) {
      s.append(function.size());
    }
  }

  List<Temp> use() {
    return Collections.emptyList();
  }

  abstract Operand rename(Function<Temp, Temp> sigma);

  abstract void render(StringBuilder s);

  @Override
  public String toString() {
    StringBuilder s = new StringBuilder();
    render(s);
    return s.toString();
  }
}
