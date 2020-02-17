package minijava.intermediate.translate;

import minijava.analysis.SymbolTable;
import minijava.backend.Platform;
import minijava.intermediate.Label;
import minijava.intermediate.Temp;
import minijava.intermediate.tree.*;
import minijava.util.Pair;

import java.util.List;

// If this becomes machine-dependent at some point then this class may be
// made abstract and its constructor moved to a factory function in
// CodeGenerator.
class Runtime {

  private final Platform platform;
  private final SymbolTable symbolTable;
  private final ClassTable classTable;

  // label of the block to raise an array bounds exception
  private Label lRaiseArrayBounds;

  Runtime(Platform platform, SymbolTable symbolTable) {
    this.platform = platform;
    this.symbolTable = symbolTable;
    this.classTable = new ClassTable(symbolTable);
  }

  // return the label (entry point) for a method
  Label methodLabel(String className, String methodName) {
    if (methodName.equals("main")
            && symbolTable.getClassEntry(className).getMethodEntry(methodName) instanceof SymbolTable.StaticMethodEntry) {
      return new Label("main");
    } else {
      return new Label(className + "$" + methodName);
    }
  }

  // return expression that defines the label for a method
  TreeExp methodAddress(TreeExp obj, String className, String methodName) {
    int index = 0;
    for (String om : classTable.getVirtualMethods(className)) {
      if (methodName.equals(om)) {
        // is in overloaded method table, i.e. dynamic call required
        return new TreeExpMem(new TreeExpBinOp(TreeExpBinOp.Op.PLUS,
                new TreeExpMem(obj),
                new TreeExpConst(index * platform.getWordSize())));
      }
      index++;
    }
    // not overloaded
    return new TreeExpName(methodLabel(className, methodName));
  }

  // return code for accessing a field of an object:
  //   MEM(<obj> + (<fieldIndex>+1)*<wordSize>)
  TreeExp fieldAddress(TreeExp obj, String className, String fieldName) {
    // find last index of field name - all other occurences are hidden
    int fieldIndex = classTable.getInstanceVariables(className).lastIndexOf(fieldName);
    assert (fieldIndex >= 0);
    int offset = platform.getWordSize() * (fieldIndex + 1);

    return new TreeExpMem(
            new TreeExpBinOp(TreeExpBinOp.Op.PLUS, obj, new TreeExpConst(offset)));
  }

  // return code for calling the print function with argument
  TreeExp callPrintln(TreeExp arg) {
    return TreeExpCall.name("_println_int", arg);
  }

  // return code for calling the write function with argument
  TreeExp callWrite(TreeExp arg) {
    return TreeExpCall.name("_write", arg);
  }

  // return code for calling the print function with argument
  TreeExp callRead() {
    return TreeExpCall.name("_read");
  }

  // return code for calling the raise function with argument
  private TreeExp callRaise(TreeExp arg) {
    return TreeExpCall.name("_raise", arg);
  }

  // return code for allocating and initializing a new array
  // i.e. allocate (length+1)*wordSize bytes and store length in first field:
  //   tlength <- <length>
  //   taddr <- CALL L_halloc((tlength+1)*<wordSize>)
  //   MEM(taddr) <- tlength
  TreeExp callNewIntArray(TreeExp length) {
    Temp taddr = new Temp();
    Temp tlength = new Temp();
    TreeStm stm = new TreeStmSeq(
            new TreeStmMove(new TreeExpTemp(tlength), length),
            new TreeStmMove(new TreeExpTemp(taddr), TreeExpCall.name("_halloc",
                    new TreeExpBinOp(TreeExpBinOp.Op.MUL,
                            new TreeExpBinOp(TreeExpBinOp.Op.PLUS, new TreeExpTemp(tlength),
                                    new TreeExpConst(1)), new TreeExpConst(platform.getWordSize())))),
            new TreeStmMove(new TreeExpMem(new TreeExpTemp(taddr)), new TreeExpTemp(tlength))
    );
    return new TreeExpESeq(stm, new TreeExpTemp(taddr));
  }

  // return code for allocating and initializing a new object
  // i.e. allocate (fieldcount+1)*wordSize bytes, which can be computed statically
  TreeExp callNew(String className) {
    List<String> virtualMethods = classTable.getVirtualMethods(className);
    int fieldcount = classTable.getInstanceVariables(className).size();

    int mtablesize = virtualMethods.size() * platform.getWordSize();
    TreeExp mtableAddr;
    TreeStm makeMethodTable;
    if (mtablesize == 0) {
      mtableAddr = new TreeExpConst(0);
      makeMethodTable = TreeStm.getNOP();
    } else {
      mtableAddr = new TreeExpTemp(new Temp());
      makeMethodTable = new TreeStmMove(mtableAddr,
              TreeExpCall.name("_halloc", new TreeExpConst(mtablesize)));

      int offset = 0;
      for (String method : virtualMethods) {
        SymbolTable.MethodEntry m = classTable.getInstanceMethodEntry(className, method);
        makeMethodTable =
                new TreeStmSeq(makeMethodTable,
                        new TreeStmMove(new TreeExpMem(
                                new TreeExpBinOp(TreeExpBinOp.Op.PLUS, mtableAddr, new TreeExpConst(offset))),
                                new TreeExpName(methodLabel(m.getDefiningClass().getClassName(), m.getMethodName()))));
        offset += platform.getWordSize();
      }
    }

    // add 1 for reserved field
    int objectsize = (fieldcount + 1) * platform.getWordSize();

    Temp objAddr = new Temp();
    return new TreeExpESeq(
            new TreeStmSeq(
                    new TreeStmSeq(
                            new TreeStmMove(new TreeExpTemp(objAddr),
                                    TreeExpCall.name("_halloc", new TreeExpConst(objectsize))), makeMethodTable),
                    new TreeStmMove(new TreeExpMem(new TreeExpTemp(objAddr)), mtableAddr)),
            new TreeExpTemp(objAddr));
  }

  private Pair<TreeStm, TreeExp> arrayDeref(TreeExp array, TreeExp index) {
    // return pair for array access array[index]:
    // 1. statements for bounds check
    //     MOVE tindex <- <index>
    //     MOVE tarray <- <array>
    //     CJUMP (tindex >= MEM(tarray)) ? Lraise : Lchecklower
    //   LcheckLower:
    //     CJUMP(tindex < 0) ? Lraise : Lderef
    //   Lraise:
    //     CALL(L_raise, -17)  // program terminates
    //   Lderef:
    // 2. expression for actual access
    //     MEM(tarray + (tindex+1)*wordSize)


    // when index is a constant: do not check lower bound, optimize access
    if (index instanceof TreeExpConst) {
      TreeExpConst cindex = (TreeExpConst) index;
      if (cindex.getValue() < 0) {
        throw new RuntimeException("Error: array access with (constant) negative index");
      }
      Temp tarray = new Temp();
      Label Lderef = new Label();
      TreeStm stm = new TreeStmSeq(
              new TreeStmMove(new TreeExpTemp(tarray), array),
              new TreeStmCJump(TreeStmCJump.Rel.GE, cindex, arrayLength(new TreeExpTemp(tarray)), lRaiseArrayBounds, Lderef),
              new TreeStmLabel(Lderef)
      );
      TreeExp access =
              new TreeExpMem(new TreeExpBinOp(TreeExpBinOp.Op.PLUS, new TreeExpTemp(tarray),
                      new TreeExpConst((cindex.getValue() + 1) * platform.getWordSize())));
      return new Pair<>(stm, access);
    } else {  // index is not a constant
      Temp tarray = new Temp();
      Temp tindex = new Temp();
      Label Lchecklower = new Label();
      Label Lderef = new Label();
      TreeStm stm = new TreeStmSeq(
              new TreeStmMove(new TreeExpTemp(tarray), array),
              new TreeStmMove(new TreeExpTemp(tindex), index),
              new TreeStmCJump(TreeStmCJump.Rel.GE, new TreeExpTemp(tindex), arrayLength(new TreeExpTemp(tarray)),
                      lRaiseArrayBounds, Lchecklower),
              new TreeStmLabel(Lchecklower),
              new TreeStmCJump(TreeStmCJump.Rel.LT, new TreeExpTemp(tindex), new TreeExpConst(0),
                      lRaiseArrayBounds, Lderef),
              new TreeStmLabel(Lderef)
      );
      TreeExp access =
              new TreeExpMem(
                      new TreeExpBinOp(TreeExpBinOp.Op.PLUS, new TreeExpTemp(tarray),
                              new TreeExpBinOp(TreeExpBinOp.Op.MUL,
                                      new TreeExpBinOp(TreeExpBinOp.Op.PLUS, new TreeExpTemp(tindex), new TreeExpConst(1)),
                                      new TreeExpConst(platform.getWordSize()))));
      return new Pair<>(stm, access);
    }
  }

  TreeExp arrayGet(TreeExp array, TreeExp index) {
    Pair<TreeStm, TreeExp> p = arrayDeref(array, index);
    return new TreeExpESeq(p.fst, p.snd);
  }

  TreeStm arrayPut(TreeExp array, TreeExp index, TreeExp data) {
    Pair<TreeStm, TreeExp> p = arrayDeref(array, index);
    return new TreeStmSeq(p.fst, new TreeStmMove(p.snd, data));
  }

  TreeExp arrayLength(TreeExp array) {
    return new TreeExpMem(array);
  }

  TreeStm newRaiseBlock() {
    lRaiseArrayBounds = new Label();
    TreeStm[] raiseBlock =
            {new TreeStmLabel(lRaiseArrayBounds),
                    new TreeStmMove(new TreeExpTemp(new Temp()), callRaise(new TreeExpConst(-17))),
                    new TreeStmJump(lRaiseArrayBounds)
            };
    return new TreeStmSeq(raiseBlock);
  }
}
