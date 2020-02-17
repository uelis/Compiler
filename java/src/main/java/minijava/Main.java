package minijava;

import minijava.analysis.SymbolTable;
import minijava.analysis.TypeCheckException;
import minijava.analysis.TypeChecking;
import minijava.backend.CodeGenerator;
import minijava.backend.MachinePrg;
import minijava.backend.Platform;
import minijava.backend.i386.I386CodeGenerator;
import minijava.backend.i386.I386Platform;
import minijava.backend.regalloc.RegAlloc;
import minijava.intermediate.canon.Canonizer;
import minijava.intermediate.canon.Tracer;
import minijava.intermediate.translate.Translator;
import minijava.intermediate.tree.TreePrg;
import minijava.parser.Lexer;
import minijava.parser.Parser;
import minijava.syntax.Program;

import java.io.*;

class Main {

  private static void writeFile(String name, String contents) {
    try {
      BufferedWriter out = new BufferedWriter(new FileWriter(name));
      out.write(contents);
      out.close();
    } catch (IOException e) {
      e.printStackTrace();
    }
  }

  public static void compile(Program program, SymbolTable symbolTable, Platform platform, CodeGenerator codeGenerator, String fileName) {
    TreePrg treePrg = new Translator(platform, symbolTable).translatePrg(program);
    TreePrg canonisedPrg = new Canonizer().canonPrg(treePrg);
    TreePrg tracedPrg = new Tracer().tracePrg(canonisedPrg);
    MachinePrg machinePrg = codeGenerator.codeGen(tracedPrg);
    RegAlloc regAllocator = new RegAlloc(codeGenerator);
    regAllocator.regAlloc(machinePrg);

    writeFile(fileName, machinePrg.renderAssembly());
  }

  /**
   * @param args the command line arguments
   */
  public static void main(String[] args) {

    if (args.length == 0) {
      System.out.println("First argument must be file name of MiniJava program.");
      System.exit(-1);
    }

    for (String filename : args) {

      String basename = (new File(filename)).getName();
      if (basename.lastIndexOf(".") >= 0) {
        basename = basename.substring(0, basename.lastIndexOf("."));
      }

      Program program;
      try (java.io.InputStream inp = new FileInputStream(filename)) {
        Parser parser = new Parser(new Lexer(new BufferedReader(new InputStreamReader(inp))));
        program = (Program) parser.parse().value;
      } catch (FileNotFoundException e) {
        throw new Error("File not found: " + filename);
      } catch (Exception e) {
        throw new Error(e.toString());
      }

      SymbolTable symbolTable = null;
      try {
        symbolTable = new SymbolTable(program);
        TypeChecking.checkPrg(symbolTable, program);

      } catch (TypeCheckException e) {
        System.out.println(e.getMessage());
        System.exit(-2);
      }

      Platform i386Platform = new I386Platform();
      CodeGenerator i386CodeGenerator = new I386CodeGenerator();
      compile(program, symbolTable, i386Platform, i386CodeGenerator, basename + ".s");
    }
  }
}
