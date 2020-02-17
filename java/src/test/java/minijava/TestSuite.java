package minijava;

import org.junit.jupiter.api.DynamicTest;
import org.junit.jupiter.api.TestFactory;
import org.junit.jupiter.api.io.TempDir;
import org.junit.jupiter.api.parallel.Execution;
import org.junit.jupiter.api.parallel.ExecutionMode;

import java.io.File;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.stream.Stream;

import static org.junit.jupiter.api.DynamicTest.dynamicTest;

@Execution(ExecutionMode.CONCURRENT)
class TestSuite {

    Stream<File> files(String sourcesDir) {
        var files = Arrays.asList(new File(sourcesDir).listFiles((d, s) -> s.endsWith(".java")));
        return files.stream();
    }

    Stream<DynamicTest> correct(Path tempDir, String sourcesDir) {
        return files(sourcesDir)
                .map(f -> dynamicTest(f.getName(), () ->
                        new CompileTest(tempDir, f.toPath()).correct()));
    }

    Stream<DynamicTest> compileError(Path tempDir, String sourcesDir) {
        return files(sourcesDir)
                .map(f -> dynamicTest(f.getName(), () ->
                        new CompileTest(tempDir, f.toPath()).shouldFailToCompile()));
    }


    Stream<DynamicTest> runtimeError(Path tempDir, String sourcesDir) {
        return files(sourcesDir)
                .map(f -> dynamicTest(f.getName(), () ->
                        new CompileTest(tempDir, f.toPath()).shouldFailRuntime()));
    }

    @TestFactory
    Stream<DynamicTest> small(@TempDir Path tempDir) {
        return correct(tempDir,"testcases/Small");
    }

    @TestFactory
    Stream<DynamicTest> medium(@TempDir Path tempDir) {
        return correct(tempDir,"testcases/Medium");
    }

    @TestFactory
    Stream<DynamicTest> large(@TempDir Path tempDir) {
        return correct(tempDir,"testcases/Large");
    }

    @TestFactory
    Stream<DynamicTest> parseErrors(@TempDir Path tempDir) {
        return compileError(tempDir, "testcases/ShouldFail/ParseErrors");
    }

    @TestFactory
    Stream<DynamicTest> typeErrors(@TempDir Path tempDir) {
        return compileError(tempDir, "testcases/ShouldFail/TypeErrors");
    }

    @TestFactory
    Stream<DynamicTest> runtimeError(@TempDir Path tempDir) {
        return runtimeError(tempDir, "testcases/ShouldFail/RuntimeErrors");
    }
}