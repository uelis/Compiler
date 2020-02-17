package minijava.backend.regalloc;

import minijava.backend.MachineInstruction;
import minijava.intermediate.Temp;
import minijava.util.DirectedGraph;
import minijava.util.Pair;

class InterferenceGraph extends DirectedGraph<Temp> {

  InterferenceGraph(Liveness live, FlowGraph flowGraph) {

    int n = flowGraph.size();

    for (int i = 0; i < n; i++) {
      MachineInstruction a = flowGraph.get(i);
      for (Temp b : a.def()) {
        addNode(b);
        for (Temp c : live.out(i)) {
          addNode(c);
          if (b.equals(c)) {
            continue;
          }
          Pair<Temp, Temp> move = a.isMoveBetweenTemps();
          if (move != null && c.equals(move.snd)) {
            continue; // do not add edge (b,c) for move b <- c
          }
          addEdge(b, c);
          addEdge(c, b);
        }
      }
    }
  }
}
