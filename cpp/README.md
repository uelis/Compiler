# C++ Implementation

The C++ implementation uses C++17. It was written after the Rust version
for comparison with modern C++.

## Compilation

Generating the `mjc` executable:
```
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make
```

## Tests

Running all tests:
```
    make test
```
The tests need python, java and gcc to run.

## Running the Compiler

Compiling the example MiniJava file `Hanoi.java`:
```
    ./mjc ../testcases/Medium/Hanoi.java
    gcc -m32 Hanoi.s ../src/runtime.c -o Hanoi
```
