#include "intermediate/tracer.h"

#include <optional>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "intermediate/canonizer.h"
#include "intermediate/names.h"
#include "intermediate/tree.h"

namespace mjc {

// Represents a basic block, which is a sequence of statements that
// - starts with a label
// - contains a number of non-jump instructions
// - ends with a jump
class BasicBlock {
 public:
  BasicBlock(Label label, std::vector<std::unique_ptr<TreeStm>> body,
             std::unique_ptr<TreeStm> transfer)
      : label_(std::move(label)),
        body_(std::move(body)),
        transfer_(std::move(transfer)) {}

  Label GetLabel() const { return label_; }

  std::vector<std::unique_ptr<TreeStm>> &GetBody() { return body_; }

  std::unique_ptr<TreeStm> &GetTransfer() { return transfer_; }

 private:
  Label label_;
  std::vector<std::unique_ptr<TreeStm>> body_;
  std::unique_ptr<TreeStm> transfer_;
};

// Decomposes a linear program into a set of basic blocks.
// (TODO: should this class be a function?)
class BlockBuilder {
 public:
  BlockBuilder(TreeFunction fun) {
    assert(!fun.body.empty());

    if (fun.body[0]->GetOp() == TreeStm::TreeStmLabelOp) {
      start_label = static_cast<TreeStmLabel &>(*fun.body[0]).GetLabel();
    }

    StartNew(start_label);

    for (auto &stm : fun.body) {
      switch (stm->GetOp()) {
        case TreeStm::TreeStmLabelOp: {
          auto l = static_cast<TreeStmLabel &>(*stm).GetLabel();
          FinishCurrent(std::make_unique<TreeStmJump>(l));
          StartNew(l);
          break;
        }
        case TreeStm::TreeStmJumpOp:
          FinishCurrent(std::move(stm));
          break;
        case TreeStm::TreeStmCJumpOp:
          FinishCurrent(std::move(stm));
          break;
        default:
          PushCurrent(std::move(stm));
      }
    }
    FinishCurrent(std::make_unique<TreeStmJump>(end_label));
  }

  std::unordered_map<Label, std::unique_ptr<BasicBlock>> blocks;
  Label start_label;
  Label end_label;

 private:
  struct PartialBlock {
    Label label;
    std::vector<std::unique_ptr<TreeStm>> body;
  };
  std::optional<PartialBlock> current;

  void StartNew(Label l) {
    current = PartialBlock{.label = l, .body = {}};
    current->body.push_back(std::make_unique<TreeStmLabel>(l));
  }

  void PushCurrent(std::unique_ptr<TreeStm> stm) {
    assert(current);
    current->body.push_back(std::move(stm));
  }

  void FinishCurrent(std::unique_ptr<TreeStm> transfer) {
    if (current) {
      Label l = current->label;
      blocks[l] = std::make_unique<BasicBlock>(
          current->label, std::move(current->body), std::move(transfer));
      current = std::nullopt;
    } else {
      // nothing
    }
  }
};

// Tracing rearranges the basic blocks into a linear program while establishing
// the tracing invariant (see beginning of file).
TreeFunction Trace(TreeFunction fun) {
  TreeFunction result{.name = fun.name,
                      .parameter_count = fun.parameter_count,
                      .body = {},
                      .return_temp = fun.return_temp};
  BlockBuilder builder(std::move(fun));
  auto start_label = builder.start_label;
  auto blocks = std::move(builder.blocks);
  auto end_label = builder.end_label;

  std::stack<Label> to_trace;
  std::unordered_set<Label> added{end_label};

  to_trace.push(start_label);

  while (to_trace.size() > 0) {
    Label l = to_trace.top();
    to_trace.pop();

    if (added.count(l) == 0) {
      // block with label l must exist
      auto block = std::move(blocks[l]);

      // if ordered ends with JMP l then remove the jump
      if (result.body.size() > 0 &&
          result.body.back()->GetOp() == TreeStm::TreeStmJumpOp) {
        TreeStmJump &jump = static_cast<TreeStmJump &>(*result.body.back());
        if (jump.GetTarget()->GetOp() == TreeExp::TreeExpNameOp) {
          TreeExpName &target = static_cast<TreeExpName &>(*jump.GetTarget());
          if (target.GetName() == l) {
            result.body.pop_back();
          }
        }
      }

      for (auto &s : block->GetBody()) {
        result.body.push_back(std::move(s));
      }

      switch (block->GetTransfer()->GetOp()) {
        case TreeStm::TreeStmJumpOp: {
          TreeStmJump &jump = static_cast<TreeStmJump &>(*block->GetTransfer());
          for (auto &target : jump.GetTargets()) {
            to_trace.push(target);
          }
          result.body.push_back(std::move(block->GetTransfer()));
          break;
        }
        case TreeStm::TreeStmCJumpOp: {
          auto &cjump = static_cast<TreeStmCJump &>(*block->GetTransfer());
          if (added.count(cjump.GetLFalse()) == 0) {
            to_trace.push(cjump.GetLTrue());
            to_trace.push(cjump.GetLFalse());
            result.body.push_back(std::move(block->GetTransfer()));
          } else if (added.count(cjump.GetLTrue()) == 0) {
            to_trace.push(cjump.GetLFalse());
            to_trace.push(cjump.GetLTrue());
            result.body.push_back(std::make_unique<TreeStmCJump>(
                TreeStmCJump::negate(cjump.GetRel()),
                std::move(cjump.GetLeft()), std::move(cjump.GetRight()),
                cjump.GetLFalse(), cjump.GetLTrue()));
          } else {
            auto dummy = Label{};
            result.body.push_back(std::make_unique<TreeStmCJump>(
                cjump.GetRel(), std::move(cjump.GetLeft()),
                std::move(cjump.GetRight()), cjump.GetLTrue(), dummy));
            result.body.push_back(std::make_unique<TreeStmLabel>(dummy));
            result.body.push_back(
                std::make_unique<TreeStmJump>(cjump.GetLFalse()));
          }
          break;
        }
        default:
          // nothing
          break;
      }
      added.insert(l);
    }
  }
  result.body.push_back(std::make_unique<TreeStmLabel>(end_label));
  return result;
}

Tracer::TracedTreeProgram Tracer::Process(Canonizer::CanonizedTreeProgram prg) {
  std::vector<TreeFunction> functions;
  for (auto &fun : prg.functions) {
    functions.push_back(Trace(std::move(fun)));
  };
  return TreeProgram{.functions = std::move(functions)};
}
}  // namespace mjc
