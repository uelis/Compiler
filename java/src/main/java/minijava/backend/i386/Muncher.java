package minijava.backend.i386;

import minijava.backend.MachineInstruction;
import minijava.backend.i386.Operand.*;
import minijava.intermediate.Label;
import minijava.intermediate.Temp;
import minijava.intermediate.tree.*;

import java.math.BigInteger;
import java.util.*;

import static minijava.backend.i386.InstrBinary.Kind.*;
import static minijava.backend.i386.InstrBinary.Kind.IMUL;
import static minijava.backend.i386.InstrJump.Kind.CALL;
import static minijava.backend.i386.InstrJump.Kind.JMP;
import static minijava.backend.i386.InstrNullary.Kind.RET;
import static minijava.backend.i386.InstrUnary.Kind.*;
import static minijava.backend.i386.Registers.*;

class Muncher {

  private ArrayList<MachineInstruction> instructions = null;

  private void emit(MachineInstruction e) {
    instructions.add(e);
  }

  I386Function codeGenFunction(TreeFunction method) {

    instructions = new ArrayList<>();
    I386Function i386Function = new I386Function(method.getName(), instructions);

    Map<Temp, Temp> calleeSaves = new HashMap<>();
    for (Temp r : Registers.calleeSave) {
      calleeSaves.put(r, new Temp());
    }

    // entry code
    emit(new InstrUnary(PUSH, new Reg(Registers.ebp)));
    emit(new InstrBinary(MOV, new Reg(Registers.ebp), new Reg(Registers.esp)));
    emit(new InstrBinary(SUB, new Reg(Registers.esp), new FrameSize(i386Function)));

    // save callee saves
    for (Temp r : Registers.calleeSave) {
      emit(new InstrBinary(MOV, new Reg(calleeSaves.get(r)), new Reg(r)));
    }

    // body
    for (TreeStm stm : method) {
      munchStm(stm);
    }

    // move result, restore callee saves
    emit(new InstrBinary(MOV, new Reg(Registers.eax), new Reg(method.getReturnTemp())));
    for (Temp r : Registers.calleeSave) {
      emit(new InstrBinary(MOV, new Reg(r), new Reg(calleeSaves.get(r))));
    }

    // exit codes
    emit(new InstrBinary(MOV, new Reg(Registers.esp), new Reg(Registers.ebp)));
    emit(new InstrUnary(POP, new Reg(Registers.ebp)));
    emit(new InstrNullary(RET));

    return i386Function;
  }

  private Operand munchSrc(TreeExp exp) {
    LinearCombination l = matchLinearCombination(exp);
    if (l != null && l.coeffs.isEmpty()) {
      return new Imm(l.constant);
    } else {
      return munchDest(exp);
    }
  }

  private Operand munchDest(TreeExp exp) {
    if (exp instanceof TreeExpTemp) {
      TreeExpTemp treeExpTemp = (TreeExpTemp) exp;
      return new Reg(treeExpTemp.getTemp());
    } else if (exp instanceof TreeExpMem) {
      TreeExpMem mem = (TreeExpMem) exp;
      return munchEffectiveAddress(munchSubExps(mem.getAddress()));
    } else {
      return munchExp(exp);
    }
  }

  // much all mems in
  private TreeExp munchSubExps(TreeExp exp) {
    if (exp instanceof TreeExpConst || exp instanceof TreeExpTemp) {
      return exp;
    }
    if (exp instanceof TreeExpBinOp) {
      TreeExpBinOp binOp = (TreeExpBinOp) exp;
      return new TreeExpBinOp(binOp.getOp(), munchSubExps(binOp.getLeft()), munchSubExps(binOp.getRight()));
    }
    Temp t = new Temp();
    emit(new InstrBinary(MOV, new Reg(t), munchExp(exp)));
    return new TreeExpTemp(t);
  }

  private void munchStm(TreeStm stm) {
    stm.accept(new TreeStmVisitor<Void>() {

      @Override
      public Void visit(TreeStmMove stmMOVE) {
        Operand d = munchDest(stmMOVE.getDst());
        Operand s = munchSrc(stmMOVE.getSrc());
        if (d instanceof Reg && s instanceof Imm && ((Imm) s).imm == 0) {
          emit(new InstrBinary(XOR, d, d));
        } else if (s instanceof Sym) {
          Operand t = new Reg(new Temp());
          emit(new InstrBinary(LEA, t, s));
          emit(new InstrBinary(MOV, d, t));
        } else if ((d instanceof Mem) && (s instanceof Mem)) {
          Operand t = new Reg(new Temp());
          emit(new InstrBinary(MOV, t, s));
          emit(new InstrBinary(MOV, d, t));
        } else {
          emit(new InstrBinary(MOV, d, s));
        }
        return null;
      }

      @Override
      public Void visit(TreeStmJump stmJUMP) {
        if (stmJUMP.getDst() instanceof TreeExpName) {
          Label l = ((TreeExpName) stmJUMP.getDst()).getLabel();
          emit(new InstrJump(JMP, l));
          return null;
        } else {
          throw new UnsupportedOperationException("Only jumps to labels are implemented!");
        }
      }

      @Override
      public Void visit(TreeStmCJump stmCJUMP) {
        InstrJump.Cond cond = null;
        LinearCombination lcl = matchLinearCombination(stmCJUMP.getLeft());
        LinearCombination lcr = matchLinearCombination(stmCJUMP.getRight());
        switch (stmCJUMP.getRel()) {
          case EQ: {
            Operand testZero = null;
            if (lcl != null && lcl.coeffs.isEmpty() && lcl.constant == 0) {
              testZero = munchExp(stmCJUMP.getRight());
            }
            if (lcr != null && lcr.coeffs.isEmpty() && lcr.constant == 0) {
              testZero = munchExp(stmCJUMP.getLeft());
            }
            if (testZero != null && !(testZero instanceof Mem)) {
              emit(new InstrBinary(TEST, testZero, testZero));
              emit(new InstrJump(InstrJump.Cond.E, stmCJUMP.getLabelTrue()));
              return null;
            }
            cond = InstrJump.Cond.E;
            break;
          }
          case NE:
            cond = InstrJump.Cond.NE;
            break;
          case LT:
            cond = InstrJump.Cond.L;
            break;
          case GT:
            cond = InstrJump.Cond.G;
            break;
          case LE:
            cond = InstrJump.Cond.LE;
            break;
          case GE:
            cond = InstrJump.Cond.GE;
            break;
          case ULT:
          case ULE:
          case UGT:
          case UGE:
            throw new UnsupportedOperationException("Unsigned conditions not supported!");
        }
        Operand l = munchExp(stmCJUMP.getLeft());
        Operand r = munchSrc(stmCJUMP.getRight());
        if (l instanceof Imm || l instanceof Sym || ((l instanceof Mem) && (r instanceof Mem))) {
          Reg t = new Reg(new Temp());
          emit(new InstrBinary(MOV, t, l));
          emit(new InstrBinary(CMP, t, r));
        } else {
          emit(new InstrBinary(CMP, l, r));
        }
        emit(new InstrJump(cond, stmCJUMP.getLabelTrue()));
        // Beachte: stmCJUMP.lfalse kommt direkt nach dieser Instruktion,
        // wegen canonisierung.
        return null;
      }

      @Override
      public Void visit(TreeStmSeq stmSEQ) {
        for (TreeStm s : stmSEQ.getStms()) {
          munchStm(s);
        }
        return null;
      }

      @Override
      public Void visit(TreeStmLabel stmLABEL) {
        emit(new InstrLabel(stmLABEL.getLabel()));
        return null;
      }
    });
  }

  private Operand munchExp(TreeExp exp) {
    LinearCombination l = matchLinearCombination(exp);
    if (l != null) {
      if (l.coeffs.isEmpty()) {
        return new Imm(l.constant);
      }
      if (l.coeffs.size() == 1 && l.constant == 0) {
        Temp t = l.coeffs.keySet().iterator().next();
        if (l.coeffs.get(t) == 1) {
          return new Reg(t);
        }
      }
      Operand.Mem op = l.asMem();
      if (op != null) {
        int n = (op.base != null ? 1 : 0) + (op.index != null ? op.scale : 0) + (op.displacement > 0 ? 1 : 0);
        if (n > 1) {
          Reg t = new Reg(new Temp());
          emit(new InstrBinary(LEA, t, op));
          return t;
        }
      }
    }

    return exp.accept(new TreeExpVisitor<Operand>() {

      @Override
      public Operand visit(TreeExpConst expCONST) {
        return new Imm(expCONST.getValue());
      }

      @Override
      public Operand visit(TreeExpName expNAME) {
        return new Sym(expNAME.getLabel().toString());
      }

      @Override
      public Operand visit(TreeExpTemp expTEMP) {
        return new Reg(expTEMP.getTemp());
      }

      @Override
      public Operand visit(TreeExpParam expPARAM) {
        return new Mem(ebp, null, null, 8 + 4 * expPARAM.getNumber());
      }

      @Override
      public Operand visit(TreeExpMem expMEM) {
        return munchEffectiveAddress(expMEM.getAddress());
      }

      @Override
      public Operand visit(TreeExpBinOp expOP) {
        InstrBinary.Kind machineOp = null;
        Operand r = munchExp(expOP.getRight());
        Operand l = munchExp(expOP.getLeft());
        switch (expOP.getOp()) {
          case PLUS:
            machineOp = ADD;
            break;
          case MUL:
            machineOp = IMUL;
            break;
          case DIV: {
            if (r instanceof Imm) {
              int d = ((Imm) r).imm;
              int ceil_log_d = 0;

              int d1 = 1;
              while (d > d1) {
                d1 = d1 * 2;
                ceil_log_d++;
              }
              boolean d_is_power_of_two = (d == d1);

              if (d == 2) {
                Reg r1 = new Reg(new Temp());
                Reg r2 = new Reg(new Temp());
                emit(new InstrBinary(MOV, r2, l));
                emit(new InstrBinary(MOV, r1, r2));
                emit(new InstrBinary(SHR, r1, new Imm(31)));
                emit(new InstrBinary(ADD, r2, r1));
                emit(new InstrBinary(SAR, r2, new Imm(1)));
                return r2;
              } else if (d_is_power_of_two) {
                Reg r1 = new Reg(new Temp());
                Reg r2 = new Reg(new Temp());
                emit(new InstrBinary(MOV, r1, l));
                emit(new InstrBinary(MOV, r2, r1));
                emit(new InstrBinary(SAR, r2, new Imm(31)));
                emit(new InstrBinary(AND, r2, new Imm(BigInteger.valueOf(2).pow(ceil_log_d).intValue() - 1)));
                emit(new InstrBinary(ADD, r1, r2));
                emit(new InstrBinary(SAR, r1, new Imm(ceil_log_d)));
                return r1;
              } else {
                int k = 31 + ceil_log_d;
                // k = 31 + ceil(log2(d))
                BigInteger[] dr = BigInteger.valueOf(2).pow(k).divideAndRemainder(BigInteger.valueOf(d));
                BigInteger m = dr[0];
                if (!dr[1].equals(BigInteger.ZERO)) {
                  m = m.add(BigInteger.valueOf(1));
                }
                // m = ceil(2^k/d)
                if (m.compareTo(BigInteger.valueOf(2).pow(31)) < 0) {
                  Reg r1 = new Reg(new Temp());
                  Reg r2 = new Reg(new Temp());
                  emit(new InstrBinary(MOV, r1, l));
                  emit(new InstrBinary(MOV, new Reg(eax), new Imm(m.intValue())));
                  emit(new InstrUnary(InstrUnary.Kind.IMUL, r1));
                  emit(new InstrBinary(MOV, r2, new Reg(edx)));
                  emit(new InstrBinary(SAR, r2, new Imm(k - 32)));
                  emit(new InstrBinary(SAR, r1, new Imm(31)));
                  emit(new InstrBinary(SUB, r2, r1));
                  return r2;
                } else {
                  int m1 = m.subtract(BigInteger.valueOf(2).pow(32)).intValue();
                  Reg r1 = new Reg(new Temp());
                  Reg r2 = new Reg(new Temp());
                  emit(new InstrBinary(MOV, r1, l));
                  emit(new InstrBinary(MOV, new Reg(eax), new Imm(m1)));
                  emit(new InstrUnary(InstrUnary.Kind.IMUL, r1));
                  emit(new InstrBinary(MOV, r2, new Reg(edx)));
                  emit(new InstrBinary(ADD, r2, l));
                  emit(new InstrBinary(SAR, r2, new Imm(k - 32)));
                  emit(new InstrBinary(SAR, r1, new Imm(31)));
                  emit(new InstrBinary(SUB, r2, r1));
                  return r2;
                }
              }
            } else {
              emit(new InstrBinary(MOV, new Reg(eax), l));
              emit(new InstrBinary(MOV, new Reg(edx), new Reg(eax))); // sign-extend eax to edx
              emit(new InstrBinary(SAR, new Reg(edx), new Imm(31)));   // because IDIV divides eax:edx by r
              emit(new InstrUnary(IDIV, r));
              Reg t = new Reg(new Temp());
              emit(new InstrBinary(MOV, t, new Reg(eax)));
              return t;
            }
          }
          case MINUS:
            machineOp = SUB;
            break;
          case AND:
            machineOp = AND;
            break;
          case OR:
            machineOp = OR;
            break;
          case LSHIFT:
            machineOp = SHL;
            break;
          case RSHIFT:
            machineOp = SHR;
            break;
          case ARSHIFT:
            machineOp = SAR;
            break;
          case XOR:
            machineOp = XOR;
            break;
          default:
            assert false;
        }

        // allgemeiner Fall
        Reg t = new Reg(new Temp());
        emit(new InstrBinary(MOV, t, l));
        emit(new InstrBinary(machineOp, t, r));
        return t;
      }

      @Override
      public Operand visit(TreeExpCall expCALL) {
        // caller-save registers do not need to be saved here, because:
        // - in the codegen-generated code, they are used only locally,
        //   i.e. never defined before and used after the call
        // - the register allocator will save all caller-save registers that are
        //   needed after the call, as they are in the def() list of CALL.

        // push arguments in reverse order
        List<TreeExp> revArgs = new LinkedList<>(expCALL.getArguments());
        Collections.reverse(revArgs);
        for (TreeExp arg : revArgs) {
          emit(new InstrUnary(PUSH, munchExp(arg)));
        }

        if (expCALL.getFunction() instanceof TreeExpName) {
          TreeExpName func = (TreeExpName) expCALL.getFunction();
          emit(new InstrJump(CALL, func.getLabel()));
        } else {
          Operand dest = munchExp(expCALL.getFunction());
          Reg t = new Reg(new Temp());
          emit(new InstrBinary(MOV, t, dest));
          emit(new InstrJump(CALL, t));
        }
        Reg t = new Reg(new Temp());
        emit(new InstrBinary(MOV, t, new Reg(eax)));
        emit(new InstrBinary(ADD, new Reg(esp), new Imm(expCALL.getArguments().size() * 4)));  // = pop all parameters
        return t;
      }

      @Override
      public Operand visit(TreeExpESeq expESEQ) {
        assert false : "Tree must be canonised!";
        return null;
      }
    });
  }

  static class LinearCombination {

    final int constant;
    final Map<Temp, Integer> coeffs;

    LinearCombination(int constant, Map<Temp, Integer> coeffs) {
      this.constant = constant;
      this.coeffs = coeffs;
    }

    LinearCombination(Integer constant) {
      this.constant = constant;
      this.coeffs = new TreeMap<>();
    }

    LinearCombination(Temp t) {
      this.constant = 0;
      this.coeffs = new TreeMap<>();
      this.coeffs.put(t, 1);
    }

    static LinearCombination add(LinearCombination l1, LinearCombination l2) {
      int sumConstant;
      TreeMap<Temp, Integer> sumCoeffs = new TreeMap<>();

      sumConstant = l1.constant + l2.constant;
      for (Temp t : l1.coeffs.keySet()) {
        sumCoeffs.put(t, l1.coeffs.get(t));
      }
      for (Temp t : l2.coeffs.keySet()) {
        Integer i = sumCoeffs.get(t);
        if (i == null) {
          i = 0;
        }
        sumCoeffs.put(t, i + l2.coeffs.get(t));
      }
      for (Temp t : new TreeSet<>(sumCoeffs.keySet())) {
        if (sumCoeffs.get(t) == 0) {
          sumCoeffs.remove(t);
        }
      }
      return new LinearCombination(sumConstant, sumCoeffs);
    }

    static LinearCombination mul(LinearCombination l1, LinearCombination l2) {

      if (l1.coeffs.size() > 0 && l2.coeffs.size() > 0) {
        return null;
      }

      int mulConstant;
      TreeMap<Temp, Integer> mulCoeffs = new TreeMap<>();

      mulConstant = l1.constant * l2.constant;
      for (Temp t : l1.coeffs.keySet()) {
        mulCoeffs.put(t, l1.coeffs.get(t) * l2.constant);
      }
      for (Temp t : l2.coeffs.keySet()) {
        mulCoeffs.put(t, l2.coeffs.get(t) * l1.constant);
      }
      return new LinearCombination(mulConstant, mulCoeffs);
    }

    Operand.Mem asMem() {
      if (coeffs.keySet().isEmpty()) {
        return new Operand.Mem(null, null, null, constant);
      }
      if (coeffs.keySet().size() == 1) {
        Iterator<Temp> it = coeffs.keySet().iterator();
        Temp index = it.next();
        int factorIndex = coeffs.get(index);

        if (factorIndex == 1 || factorIndex == 2 || factorIndex == 4 || factorIndex == 8) {
          return new Operand.Mem(null, factorIndex, index, constant);
        } else {
          return null;
        }
      }
      if (coeffs.keySet().size() == 2) {
        Iterator<Temp> it = coeffs.keySet().iterator();
        Temp base = it.next();
        Temp index = it.next();

        if (coeffs.get(base) != 1) {
          Temp t = base;
          index = base;
          base = t;
        }
        int factorBase = coeffs.get(base);
        int factorIndex = coeffs.get(index);

        if (factorBase == 1 && (factorIndex == 1 || factorIndex == 2 || factorIndex == 4 || factorIndex == 8)) {
          return new Operand.Mem(base, factorIndex, index, constant);
        } else {
          return null;
        }
      }
      return null;
    }
  }

  private static LinearCombination matchLinearCombination(TreeExp exp) {
    if (exp instanceof TreeExpConst) {
      TreeExpConst treeExpConst = (TreeExpConst) exp;
      return new LinearCombination(treeExpConst.getValue());
    }
    if (exp instanceof TreeExpTemp) {
      TreeExpTemp treeExpTemp = (TreeExpTemp) exp;
      return new LinearCombination(treeExpTemp.getTemp());
    }
    if (exp instanceof TreeExpBinOp) {
      TreeExpBinOp binOp = (TreeExpBinOp) exp;
      LinearCombination l = matchLinearCombination(binOp.getLeft());
      LinearCombination r = matchLinearCombination(binOp.getRight());
      if (l == null || r == null) {
        return null;
      }
      switch (binOp.getOp()) {
        case PLUS:
          return LinearCombination.add(l, r);
        case MINUS:
          // note: the mul can never return null
          return LinearCombination.add(l, LinearCombination.mul(r, new LinearCombination(-1)));
        case MUL:
          return LinearCombination.mul(l, r);
      }
      return null;
    }
    return null;
  }

  // [base] + scale*[index] + displ;
  private Operand munchEffectiveAddress(TreeExp exp) {

    LinearCombination l = matchLinearCombination(exp);
    Mem op = null;
    if (l != null) {
      op = l.asMem();
    }
    if (op != null) {
      return op;
    } else {
      Temp t = new Temp();
      emit(new InstrBinary(MOV, new Reg(t), munchExp(exp)));
      return new Mem(t);
    }
  }
}
