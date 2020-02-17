package minijava.backend.i386;

import minijava.backend.MachineFunction;
import minijava.backend.MachineInstruction;
import minijava.intermediate.Label;
import minijava.intermediate.Temp;
import minijava.util.Pair;

import java.util.*;
import java.util.function.Function;

import static minijava.backend.i386.InstrBinary.Kind.MOV;

final class I386Function implements MachineFunction {

  private final Label name;
  private int localsInStack;
  private List<MachineInstruction> body;

  I386Function(Label name, List<MachineInstruction> body) {
    this.name = name;
    localsInStack = 0;
    this.body = body;
  }

  public Label getName() {
    return name;
  }

  public int size() {
    return localsInStack * 4;
  }

  private Operand addLocalOnStack() {
    localsInStack++;
    return new Operand.Mem(Registers.ebp, null, null, -4 * localsInStack);
  }

  @Override
  public Iterator<MachineInstruction> iterator() {
    return body.iterator();
  }

  @Override
  public void rename(Function<Temp, Temp> sigma) {
    for (MachineInstruction i : body) {
      i.rename(sigma);
    }
    ListIterator<MachineInstruction> iterator = body.listIterator();
    while (iterator.hasNext()) {
      MachineInstruction i = iterator.next();
      i.rename(sigma);
      Pair<Temp, Temp> move = i.isMoveBetweenTemps();
      // do not add noop-moves
      if (move != null && move.fst.equals(move.snd)) {
        iterator.remove();
      }
    }
  }

  @Override
  public void spill(List<Temp> toSpill) {

    Map<Temp, Operand> spills = new HashMap<>(toSpill.size());

    // allocate space for the spilled temporaries
    for (Temp t : toSpill) {
      spills.put(t, this.addLocalOnStack());
    }

    List<MachineInstruction> newBody = new ArrayList<>(body.size());
    Map<Temp, Temp> freshNames = new HashMap<>();
    for (MachineInstruction i : body) {

      freshNames.clear();
      Pair<Temp, Temp> move = i.isMoveBetweenTemps();
      if (move != null && !(toSpill.contains(move.fst) && toSpill.contains(move.snd))) {
        newBody.add(new InstrBinary(MOV,
                spills.getOrDefault(move.fst, new Operand.Reg(move.fst)),
                spills.getOrDefault(move.snd, new Operand.Reg(move.snd))));
      } else {
        List<MachineInstruction> loadInstructions = new LinkedList<>();
        // TODO: Spezialfaelle wie push koennen hier noch optimiert werden!
        for (Temp u : i.use()) {
          if (spills.keySet().contains(u)) {
            Temp f = freshNames.computeIfAbsent(u, t -> new Temp());
            loadInstructions.add(new InstrBinary(MOV, new Operand.Reg(f), spills.get(u)));
          }
        }

        List<MachineInstruction> saveInstructions = new LinkedList<>();
        // TODO: Spezialfaelle wie push koennen hier noch optimiert werden!
        for (Temp d : i.def()) {
          if (spills.keySet().contains(d)) {
            Temp f = freshNames.computeIfAbsent(d, t -> new Temp());
            saveInstructions.add(new InstrBinary(MOV, spills.get(d), new Operand.Reg(f)));
          }
        }

        newBody.addAll(loadInstructions);
        i.rename(t -> freshNames.getOrDefault(t, t));
        newBody.add(i);
        newBody.addAll(saveInstructions);
      }
    }
    body = newBody;
  }
}
