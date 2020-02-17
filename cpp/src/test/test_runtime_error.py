#!/bin/python3
import subprocess
import tempfile
import os
import sys

if len(sys.argv) < 4:
    print("usage: test_runtime_error mjc runtime.c input.java")
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


def run_bin(base, log):
    inp = None
    if os.path.exists(base + ".in"):
        inp = open(base + ".in", "r")
    out = open(log, "w")
    bin = subprocess.run(["./" + base + ".bin"],
                         stdin=inp,
                         stdout=out,
                         stderr=subprocess.DEVNULL)
    return bin.returncode == 0


def should_fail_runtime(java):
    base, _ = os.path.splitext(java)
    print("Testing (should fail, runtime) " + base + ": ", end="", flush=True)
    if not compile_bin(base):
        return False
    bin = subprocess.run(["./" + base + ".bin"],
                         stdin=subprocess.DEVNULL,
                         stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)
    return bin.returncode != 0


with tempfile.TemporaryDirectory() as tmpdirname:
    print('created temporary directory', tmpdirname)
    os.chdir(tmpdirname)
    subprocess.run(["cp", input_file, "."])
    java = os.path.basename(input_file)
    ok = should_fail_runtime(java)
    tr(ok)
    sys.exit(0 if ok else 1)
