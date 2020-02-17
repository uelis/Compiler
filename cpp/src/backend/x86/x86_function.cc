#include "backend/x86/x86_function.h"

#include <map>
#include <memory>
#include <utility>

#include "backend/x86/x86_instr.h"
#include "backend/x86/x86_registers.h"
#include "backend/x86/x86_target.h"

namespace mjc {

using R = X86Register;  // TODO

X86Function::X86Function(Label name,
                         std::vector<std::unique_ptr<X86Instr>> body)
    : name_(std::move(name)), body_(std::move(body)) {
  assert(
      std::all_of(body_.begin(), body_.end(), [](auto &x) { return (bool)x; }));
}

const Label &X86Function::GetName() const { return name_; }
const std::vector<std::unique_ptr<X86Instr>> &X86Function::GetBody() const {
  return body_;
}

unsigned X86Function::GetFrameSize() const { return frame_size_; }

void X86Function::rename(std::function<R(R)> &sigma) {
  std::vector<std::unique_ptr<X86Instr>> new_body;
  new_body.reserve(body_.size());

  for (auto &i : body_) {
    i->rename(sigma);
    auto p = i->IsMoveBetweenTemps();
    if (p && p->first == p->second) continue;
    new_body.push_back(std::move(i));
  }
  std::exchange(body_, std::move(new_body));
};

Operand X86Function::AddLocalOnStack() {
  frame_size_ += X86Target::WORD_SIZE;
  return Operand::Mem(EBP, -((int)frame_size_));
}

void X86Function::spill(std::vector<R> &toSpill) {
  std::unordered_map<R, Operand> spills;
  const auto spill_op = [&spills](R t) {
    auto it = spills.find(t);
    return (it == spills.end()) ? Operand::Reg(t) : it->second;
  };

  for (auto t : toSpill) {
    spills.insert({t, AddLocalOnStack()});
  }

  std::map<R, R> fresh_idents;
  const auto get_fresh_ident = [&](auto t) {
    auto it = fresh_idents.find(t);
    if (it != fresh_idents.end())
      return Operand::Reg(it->second);
    else {
      auto r = Temp{};
      fresh_idents[t] = r;
      return Operand::Reg(r);
    }
  };

  std::vector<std::unique_ptr<X86Instr>> new_body;
  new_body.reserve(body_.size());

  for (auto &i : body_) {
    if (auto p = i->IsMoveBetweenTemps()) {
      auto dst_op = spill_op(p->first);
      auto src_op = spill_op(p->second);
      if (dst_op.IsReg() || src_op.IsReg()) {
        new_body.push_back(std::make_unique<BinaryInstr>(MOV, dst_op, src_op));
      } else {
        auto r = Operand::Reg(Temp{});
        new_body.push_back(std::make_unique<BinaryInstr>(MOV, r, src_op));
        new_body.push_back(std::make_unique<BinaryInstr>(MOV, dst_op, r));
      }
      continue;
    }

    auto uses = i->Uses();
    auto defs = i->Defs();

    if (uses.size() + defs.size() == 0) {
      new_body.push_back(std::move(i));
      continue;
    }

    fresh_idents.clear();

    for (auto u : uses) {
      auto uit = spills.find(u);
      if (uit != spills.end()) {
        auto r = get_fresh_ident(u);
        new_body.push_back(std::make_unique<BinaryInstr>(MOV, r, uit->second));
      }
    }

    new_body.push_back(std::move(i));
    auto &j = new_body.back();

    for (auto d : defs) {
      auto dit = spills.find(d);
      if (dit != spills.end()) {
        auto r = get_fresh_ident(d);
        new_body.push_back(std::make_unique<BinaryInstr>(MOV, dit->second, r));
      }
    }

    // rename at end
    std::function<R(R)> sigma = [&](auto t) {
      auto it = fresh_idents.find(t);
      return (it != fresh_idents.end()) ? it->second : t;
    };
    j->rename(sigma);
  }

  std::exchange(body_, std::move(new_body));
};

}  // namespace mjc
