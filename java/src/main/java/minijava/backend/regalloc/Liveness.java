package minijava.backend.regalloc;

import minijava.intermediate.Temp;

import java.util.ArrayList;
import java.util.Set;
import java.util.TreeSet;

final class Liveness {

  private final ArrayList<Set<Temp>> in;
  private final ArrayList<Set<Temp>> out;

  Liveness(FlowGraph flowGraph) {
    int n = flowGraph.size();

    in = new ArrayList<>(n);
    out = new ArrayList<>(n);
    for (int a = 0; a < n; a++) {
      out.add(new TreeSet<>());
      in.add(new TreeSet<>());
    }

    boolean change = true;
    while (change) {
      change = false;
      for (int a = n - 1; a >= 0; a--) {
        int k = in(a).size();
        for (Integer m : flowGraph.successors(a)) {
          for (Temp t : in(m)) {
            if (out(a).add(t)) {
              change = true;
              in(a).add(t);
            }
          }
        }
        flowGraph.get(a).def().forEach(in(a)::remove);
        flowGraph.get(a).use().forEach(in(a)::add);
        change |= in(a).size() > k;
      }
    }
  }

  public Set<Temp> in(int i) {
    return in.get(i);
  }

  public Set<Temp> out(int i) {
    return out.get(i);
  }
}
