#!/bin/python3
import subprocess
import tempfile
import os
import sys

if len(sys.argv) < 4:
    print("usage: test_compile_error mjc runtime.c input.java")
    sys.exit(1)

mjc = sys.argv[1]
runtime_c = sys.argv[2]
input_file = sys.argv[3]


def tr(f):
    if f:
        print("OK")
    else:
        print("FAIL")


def compile_bin(base):
    assembler = base + ".s"
    if os.path.exists(assembler):
        os.remove(assembler)
    out = open(base + ".s", "w")
    bin = subprocess.run([mjc, base + ".java"],
                         stdout=out,
                         stderr=subprocess.DEVNULL)
    if bin.returncode != 0:
        return False

    gcc = subprocess.run(["gcc", "-m32", assembler, runtime_c, "-o", base + ".bin"],
                         stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)
    return gcc.returncode == 0


def should_fail_compile(java):
    base, _ = os.path.splitext(java)
    print("Testing (should fail, compiletime) " +
          base + ": ", end="", flush=True)
    return not compile_bin(base)


with tempfile.TemporaryDirectory() as tmpdirname:
    print('created temporary directory', tmpdirname)
    os.chdir(tmpdirname)
    subprocess.run(["cp", input_file, "."])
    java = os.path.basename(input_file)
    ok = should_fail_compile(java)
    tr(ok)
    sys.exit(0 if ok else 1)
