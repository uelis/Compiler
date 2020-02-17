package minijava.backend.regalloc;

import minijava.backend.CodeGenerator;
import minijava.backend.MachineFunction;
import minijava.backend.MachinePrg;
import minijava.intermediate.Temp;

import java.util.*;

public class RegAlloc {

  private final Set<Temp> allColours;
  private final Set<Temp> availableColours;

  public RegAlloc(CodeGenerator codeGenerator) {
    allColours = new HashSet<>(codeGenerator.getAllRegisters());
    availableColours = new HashSet<>(codeGenerator.getGeneralPurposeRegisters());
  }

  public void regAlloc(MachinePrg prg) {
    prg.forEach(this::regAlloc);
  }

  private void regAlloc(MachineFunction function) {
    InterferenceGraph interferenceGraph = build(function);
    Deque<Temp> stack = simplifyAndSpill(interferenceGraph);
    Colouring result = select(interferenceGraph, stack);

    if (result.actualSpills.isEmpty()) {
      final Temp def = availableColours.iterator().next();
      function.rename(t -> result.colouring.getOrDefault(t, def));
    } else {
      function.spill(result.actualSpills);
      regAlloc(function);
    }
  }

  private InterferenceGraph build(MachineFunction function) {
    FlowGraph flowProc = new FlowGraph(function);
    Liveness livenessInfo = new Liveness(flowProc);
    InterferenceGraph interferenceGraph = new InterferenceGraph(livenessInfo, flowProc);
    Set<Temp> ignore = new HashSet<>(allColours);
    ignore.removeAll(availableColours);
    ignore.forEach(interferenceGraph::removeNode);
    return interferenceGraph;
  }

  private Deque<Temp> simplifyAndSpill(InterferenceGraph interferenceGraph) {
    Deque<Temp> stack = new ArrayDeque<>(interferenceGraph.nodeSet().size());

    final int k = availableColours.size();
    Deque<Temp> lowDegree = new ArrayDeque<>();
    Map<Temp, Integer> highDegree = new HashMap<>();

    for (Temp n : interferenceGraph.nodeSet()) {
      if (availableColours.contains(n)) {
        continue;
      }

      int d = interferenceGraph.successors(n).size();
      if (d < k) {
        lowDegree.push(n);
      } else {
        highDegree.put(n, d);
      }
    }

    while (lowDegree.size() + highDegree.size() > 0) {

      Temp nextNode = null;
      if (lowDegree.size() > 0) {
        nextNode = lowDegree.pop();
      } else {
        int maxDegree = -1;
        for (Map.Entry<Temp, Integer> e : highDegree.entrySet()) {
          Temp n = e.getKey();
          int deg = e.getValue();
          if (deg > maxDegree) {
            maxDegree = deg;
            nextNode = n;
          }
        }
        highDegree.remove(nextNode);
      }

      stack.push(nextNode);
      for (Temp m : interferenceGraph.successors(nextNode)) {
        Integer d = highDegree.get(m);
        if (d != null) {
          if (d == k) {
            lowDegree.push(m);
            highDegree.remove(m);
          } else {
            highDegree.put(m, d - 1);
          }
        }
      }
    }
    return stack;
  }

  private class Colouring {
    final Map<Temp, Temp> colouring = new HashMap<>();
    final List<Temp> actualSpills = new ArrayList<>();
  }

  private Colouring select(InterferenceGraph interferenceGraph, Deque<Temp> stack) {
    Colouring result = new Colouring();

    allColours.forEach(c -> result.colouring.put(c, c));

    while (stack.size() > 0) {
      Temp s = stack.pop();
      Set<Temp> possibleColours = new HashSet<>(availableColours);
      interferenceGraph.successors(s).forEach(t -> possibleColours.remove(result.colouring.get(t)));
      Iterator<Temp> it = possibleColours.iterator();
      if (it.hasNext()) {
        result.colouring.put(s, it.next());
      } else { // actual spill!
        result.actualSpills.add(s);
      }
    }
    return result;
  }
}
