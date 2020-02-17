package minijava.backend.i386;

import minijava.backend.MachineInstruction;

interface I386Instruction extends MachineInstruction {

  void render(StringBuilder s);
}
