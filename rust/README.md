# Rust Implementation

It was written before the C++ version and should be largely
equivalent to it, with regard to algorithms and data structures.

It does not give nice error messages for syntax errors or type
errors, like the C++ version does. Non-essential parts of the
compiler are usually only implemented once in one of the versions.

## Compilation

Generating the `mjc` executable:
```
    cargo build --release
```

## Tests

Running all tests:
```
    cargo test
```
The rests need java and gcc to run.

For each test case, all generated files are written to
`/tmp/testcase_name`, so the tests will probably only
work on a unix-like OS.

## Running the Compiler

Compiling the example MiniJava file `Hanoi.java`:
```
    ./target/release/mjc test/Hanoi.java
    gcc -m32 Hanoi.s src/runtime.c -o Hanoi
```
