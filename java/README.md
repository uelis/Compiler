# Java Implementation

It was written before the Rust and C++ versions. The algorithms
are largely the same. Some experiments in the backend were only
made in the Java version.

## Building and Testing

Generating `mjc.jar`:
```
    ./gradlew build
```
This will also run all tests.
The tests need gcc to run.

## Running the Compiler

Compiling the example MiniJava file `Hanoi.java`:
```
    java -jar build/libs/mjc.jar testcases/Medium/Hanoi.java
    gcc -m32 Hanoi.s src/runtime.c -o Hanoi
```
