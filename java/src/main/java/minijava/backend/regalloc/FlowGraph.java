package minijava.backend.regalloc;

import minijava.backend.MachineFunction;
import minijava.backend.MachineInstruction;
import minijava.intermediate.Label;
import minijava.util.DirectedGraph;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

class FlowGraph extends DirectedGraph<Integer> {

  private final ArrayList<MachineInstruction> instructions;

  FlowGraph(MachineFunction function) {
    instructions = new ArrayList<>();
    function.forEach(instructions::add);

    // make nodes and find the definition points for labels
    Map<Label, Integer> labelDef = new HashMap<>();
    for (int n = 0; n < instructions.size(); n++) {
      addNode(n);
      Label l = instructions.get(n).isLabel();
      if (l != null) {
        labelDef.put(l, n);
      }
    }

    // add edges
    for (int n = 0; n < instructions.size(); n++) {
      MachineInstruction i = instructions.get(n);
      if (i.isFallThrough() && n + 1 < instructions.size()) {
        addEdge(n, n + 1);
      }
      for (Label l : i.jumps()) {
        addEdge(n, labelDef.get(l));
      }
    }
  }

  public int size() {
    return instructions.size();
  }

  public MachineInstruction get(int i) {
    return instructions.get(i);
  }

}
