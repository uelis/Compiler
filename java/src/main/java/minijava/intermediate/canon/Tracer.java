package minijava.intermediate.canon;

import minijava.intermediate.Label;
import minijava.intermediate.tree.*;

import java.util.*;

public class Tracer {

  public TreePrg tracePrg(TreePrg prg) {
    List<TreeFunction> methods = new ArrayList<>();
    for (TreeFunction m : prg) {
      methods.add(traceFunction(m));
    }
    return new TreePrg(methods);
  }

  private TreeFunction traceFunction(TreeFunction function) {

    Label endLabel = new Label();
    LinkedHashMap<Label, LinkedList<TreeStm>> blocks = buildBlocks(function, endLabel);
    List<LinkedList<TreeStm>> orderedBlocks = traces(blocks, endLabel);

    List<TreeStm> stms = new ArrayList<>();
    for (List<TreeStm> block : orderedBlocks) {
      stms.addAll(block);
    }
    stms.add(new TreeStmLabel(endLabel));

    return new TreeFunction(function.getName(), function.getNumberOfParameters(), stms, function.getReturnTemp());
  }

  /**
   * Computes the blocks of the given stms and returns them in the order in
   * which they were defined.
   */
  private LinkedHashMap<Label, LinkedList<TreeStm>> buildBlocks(TreeFunction method, Label endLabel) {
    LinkedHashMap<Label, LinkedList<TreeStm>> blocks = new LinkedHashMap<>();

    if (!method.iterator().hasNext()) {
      return blocks;
    }

    LinkedList<TreeStm> currentBlock = null; // null means no active block
    Label currentLabel;

    // If the code does not start with a LABEL then we invent a
    // start label and begin first block before we enter the loop here.
    // Otherwise the LABEL instruction will do that in the loop.
    if (!(method.iterator().next() instanceof TreeStmLabel)) {
      currentBlock = new LinkedList<>();
      currentLabel = new Label();
      currentBlock.add(new TreeStmLabel(currentLabel));
      blocks.put(currentLabel, currentBlock);
    }

    for (TreeStm stm : method) {

      // LABEL indicates the start of a new block
      if (stm instanceof TreeStmLabel) {
        currentLabel = ((TreeStmLabel) stm).getLabel();
        // is there an active block that was not ended by a jump?
        if (currentBlock != null) {
          currentBlock.add(new TreeStmJump(currentLabel));
        }
        // start new block
        currentBlock = new LinkedList<>();
        blocks.put(currentLabel, currentBlock);
        currentBlock.add(stm);
      } else // a jump indicates the end of a block
        if (stm instanceof TreeStmJump || stm instanceof TreeStmCJump) {
          assert currentBlock != null : "dead code found during tracing";
          currentBlock.add(stm);
          currentBlock = null;
        } else {
          assert currentBlock != null : "dead code found during tracing";
          currentBlock.add(stm);
        }
    }
    // was the last block ended by a jump instruction?
    if (currentBlock != null) {
      currentBlock.add(new TreeStmJump(endLabel));
    }

    return blocks;
  }

  private List<LinkedList<TreeStm>> traces(LinkedHashMap<Label, LinkedList<TreeStm>> blocks, Label endLabel) {
    LinkedList<LinkedList<TreeStm>> orderedBlocks = new LinkedList<>();
    Set<Label> added = new HashSet<>();
    Deque<Label> toTrace = new LinkedList<>();

    if (blocks.keySet().isEmpty()) {
      return orderedBlocks;
    }

    // start with first label
    toTrace.add(blocks.keySet().iterator().next());
    added.add(endLabel);

    while (!toTrace.isEmpty()) {
      Label l = toTrace.removeFirst();
      if (added.contains(l)) {
        continue;
      }
      LinkedList<TreeStm> block = blocks.get(l);

      addBlock(orderedBlocks, l, block);
      added.add(l);

      // make a case distinction on the kind of jump that ends the current block
      TreeStm lastInBlock = block.getLast();

      if (lastInBlock instanceof TreeStmJump) {
        for (Label ldest : ((TreeStmJump) lastInBlock).getPossibleTargets()) {
          toTrace.addFirst(ldest);
        }
      } else if (lastInBlock instanceof TreeStmCJump) {
        TreeStmCJump s = (TreeStmCJump) lastInBlock;
        if (!added.contains(s.getLabelFalse())) {
          toTrace.addFirst(s.getLabelTrue());
          toTrace.addFirst(s.getLabelFalse());
        } else if (!added.contains(s.getLabelTrue())) {
          // negate condition and swap labels
          // in order to enforce that the label for the 'false'-branch follows
          // the CJUMP immediately
          block.removeLast();
          block.add(new TreeStmCJump(s.getRel().neg(), s.getLeft(), s.getRight(), s.getLabelFalse(), s.getLabelTrue()));
          toTrace.addFirst(s.getLabelFalse());
          toTrace.addFirst(s.getLabelTrue());
        } else {
          // enforce trivially that the label for the 'false'-branch follows
          // the CJUMP immediately
          Label l1 = new Label();
          block.removeLast();
          block.add(new TreeStmCJump(s.getRel(), s.getLeft(), s.getRight(), s.getLabelTrue(), l1));
          block.add(new TreeStmLabel(l1));
          block.add(new TreeStmJump(s.getLabelFalse()));
        }
      }
    }

    addBlock(orderedBlocks, endLabel, new LinkedList<>());

    return orderedBlocks;
  }

  private void addBlock(LinkedList<LinkedList<TreeStm>> orderedBlocks, Label l, LinkedList<TreeStm> block) {
    LinkedList<TreeStm> lastBlock = orderedBlocks.peekLast();
    if (lastBlock != null && lastBlock.getLast() instanceof TreeStmJump) {
      TreeStmJump treeStmJump = (TreeStmJump) lastBlock.getLast();
      if (treeStmJump.getPossibleTargets().equals(Collections.singletonList(l))) {
        lastBlock.removeLast();
      }
    }
    orderedBlocks.add(block);
  }
}
