#include <iostream>
#include <fstream>
#include <memory>
#include <filesystem>

#include "intermediate/canonizer.h"
#include "intermediate/minijava_to_tree.h"
#include "intermediate/tracer.h"
#include "minijava/ast.h"
#include "minijava/error.h"
#include "minijava/parser_context.h"
#include "minijava/symbol.h"
#include "minijava/typecheck.h"

#include "backend/x86/x86_prg.h"
#include "backend/x86/x86_target.h"

#include "backend/regalloc.h"

int main(int argc, char *argv[]) {
  using namespace mjc;

  if (argc != 2) {
    std::cerr << "Usage: mjc <filename.java>" << std::endl;
    return 1;
  }

  auto input = std::filesystem::path{argv[1]};
  auto target = input.filename().replace_extension(".s");
  try {
    // parsing
    auto parser_context = ParserContext{input};
    auto &prg = parser_context.Parse();
    
    // type checking
    auto symbols = SymbolTable{prg};
    Typecheck(symbols, prg);

    // translation to intermediate language
    auto tree = MinijavaToTree<X86Target>{symbols}.Process(prg);
    auto canonized = Canonizer::Process(std::move(tree));
    auto traced = Tracer::Process(std::move(canonized));

    // instruction selection and register allocation
    auto assem = X86Target::CodeGen(traced);
    RegAlloc<X86Target>{}.Process(assem);

    auto out = std::ofstream{target};
    out << assem;

  } catch (CompileError &e) {
    e.report(input);
    return 1;
  }

  return 0;
}
