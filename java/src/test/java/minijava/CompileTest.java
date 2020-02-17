package minijava;

import minijava.analysis.SymbolTable;
import minijava.analysis.TypeChecking;
import minijava.backend.i386.I386CodeGenerator;
import minijava.backend.i386.I386Platform;
import minijava.parser.Lexer;
import minijava.parser.Parser;
import minijava.syntax.Program;
import org.apache.commons.io.FileUtils;
import org.apache.commons.io.FilenameUtils;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.*;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;

import static java.util.Collections.singletonList;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.DynamicTest.dynamicTest;

class CompileTest {

    private Path testDir;

    private final String base;
    private final Path java;
    private final Path asm;
    private final Path bin;
    private final Path input;
    private final Path javalog;
    private final Path binlog;

    public CompileTest(Path tempDir, Path source) throws IOException {
        base = FilenameUtils.getBaseName(source.toString());

        testDir = Files.createTempDirectory(tempDir, base);
        Path workingCopy = testDir.resolve(source.getFileName());
        Files.copy(source, workingCopy);

        java = testDir.resolve(base + ".java");
        asm = testDir.resolve(base + ".s");
        bin = testDir.resolve(base + ".bin");
        input = source.resolveSibling(base + ".in");
        binlog = testDir.resolve(base + ".bin.log");
        javalog = testDir.resolve(base + ".java.log");
    }

    void compileJava() throws IOException, InterruptedException {
        ProcessBuilder javac =
                new ProcessBuilder("javac", java.toString());
        javac.directory(testDir.toFile());
        assert (javac.start().waitFor() == 0);
    }

    void runJava() throws IOException, InterruptedException {
        ProcessBuilder java =
                new ProcessBuilder("java", base);
        java.directory(testDir.toFile());
        java.redirectOutput(javalog.toFile());
        if (Files.exists(input)) {
            java.redirectInput(input.toFile());
        }
        assert (java.start().waitFor() == 0);
    }

    void compileBin()  throws IOException, InterruptedException {
        try (InputStream inp = new FileInputStream(testDir.resolve(java).toFile())) {

            /* create a parsing object */
            Parser parser = new Parser(new Lexer(new BufferedReader(new InputStreamReader(inp))));
            Program program = (Program) parser.parse().value;
            SymbolTable symbolTable = new SymbolTable(program);
            TypeChecking.checkPrg(symbolTable, program);
            Main.compile(program, symbolTable, new I386Platform(), new I386CodeGenerator(), asm.toString());
        } catch (Exception e) {
            assert false : "Cannot compile: " + e;
        }

        ProcessBuilder gcc =
                new ProcessBuilder("gcc", "-m32",
                        asm.toString(),
                        new File("src/runtime.c").getAbsolutePath(),
                        "-o", bin.toString());
        gcc.directory(testDir.toFile());
        assert (gcc.start().waitFor() == 0) : "gcc compile";
    }

    void runBin() throws IOException, InterruptedException {

        ProcessBuilder exe =
                new ProcessBuilder(bin.toString());
        exe.directory(testDir.toFile());
        exe.redirectOutput(binlog.toFile());
        if (Files.exists(input)) {
            exe.redirectInput(input.toFile());
        }
        assert (exe.start().waitFor() == 0) : "execution";
    }

    void correct() throws IOException, InterruptedException {
        compileJava();
        runJava();
        compileBin();
        runBin();
        assert (FileUtils.contentEquals(javalog.toFile(), binlog.toFile())) : "output";
    }

    void shouldFailToCompile() throws IOException, InterruptedException {

        try (InputStream inp = new FileInputStream(testDir.resolve(java).toFile())) {

            /* create a parsing object */
            Parser parser = new Parser(new Lexer(new BufferedReader(new InputStreamReader(inp))));
            Program program = (Program) parser.parse().value;
            SymbolTable symbolTable = new SymbolTable(program);
            TypeChecking.checkPrg(symbolTable, program);
            Main.compile(program, symbolTable, new I386Platform(), new I386CodeGenerator(), asm.toString());
        } catch (Throwable e) {
            return;
        }
        assert false : "Compilation did not fail";
    }

    void shouldFailRuntime() throws IOException, InterruptedException {
        compileBin();

        ProcessBuilder exe =
                new ProcessBuilder(bin.toString());
        exe.directory(testDir.toFile());
        exe.redirectOutput(binlog.toFile());
        if (Files.exists(input)) {
            exe.redirectInput(input.toFile());
        }
        assert (exe.start().waitFor() != 0) : "execution should fail";
    }
}