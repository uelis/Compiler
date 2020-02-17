package minijava.backend.i386;

import minijava.backend.CodeGenerator;
import minijava.intermediate.Temp;
import minijava.intermediate.tree.TreeFunction;
import minijava.intermediate.tree.TreePrg;

import java.util.ArrayList;
import java.util.List;

public class I386CodeGenerator implements CodeGenerator {

  @Override
  public List<Temp> getAllRegisters() {
    return Registers.all;
  }

  @Override
  public List<Temp> getGeneralPurposeRegisters() {
    return Registers.generalPurpose;
  }

  @Override
  public I386Prg codeGen(TreePrg prg) {
    List<I386Function> functions = new ArrayList<>();
    for (TreeFunction f : prg) {
      Muncher cg = new Muncher();
      functions.add(cg.codeGenFunction(f));
    }
    return new I386Prg(functions);
  }

}
