#!/bin/python3
import subprocess
import tempfile
import os
import sys

if len(sys.argv) < 4:
    print("usage: compile mjc runtime.c input.java")
    sys.exit(1)

mjc = sys.argv[1]
runtime_c = sys.argv[2]
input_file = sys.argv[3]


def tr(f):
    if f:
        print("OK")
    else:
        print("FAIL")


def compile_java(base):
    javac = subprocess.run(["javac", base + ".java"],
                           stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL)
    return javac.returncode == 0


def compile_bin(base):
    assembler = base + ".s"
    if os.path.exists(assembler):
        os.remove(assembler)
    bin = subprocess.run([mjc, base + ".java"],
                         stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)
    if bin.returncode != 0:
        return False

    gcc = subprocess.run(["gcc", "-m32", assembler, runtime_c, "-o", base + ".bin"],
                         stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)
    return gcc.returncode == 0


def run_java(base, log):
    inp = None
    if os.path.exists(base + ".in"):
        inp = open(base + ".in", "r")
    out = open(log, "w")
    java = subprocess.run(["java", base],
                          stdin=inp,
                          stdout=out,
                          stderr=subprocess.DEVNULL)
    return java.returncode == 0


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


def test_file(java):
    base, _ = os.path.splitext(java)
    javalog = base + ".java.log"
    binlog = base + ".bin.log"
    print("Testing (x86) " + base + ": ", end="", flush=True)

    if not compile_java(base):
        return False
    if not run_java(base, javalog):
        return False

    if not compile_bin(base):
        return False
    if not run_bin(base, binlog):
        return False

    diff = subprocess.run(["diff", javalog, binlog],
                          stdin=subprocess.DEVNULL,
                          stdout=subprocess.DEVNULL,
                          stderr=subprocess.DEVNULL)
    return diff.returncode == 0


with tempfile.TemporaryDirectory() as tmpdirname:
    print('created temporary directory', tmpdirname)
    os.chdir(tmpdirname)
    subprocess.run(["cp", input_file, "."])
    base, _ = os.path.splitext(input_file)
    inp = base + ".in"
    if os.path.exists(inp):
        subprocess.run(["cp", inp, "."])
    java = os.path.basename(input_file)
    ok = test_file(java)
    tr(ok)
    sys.exit(0 if ok else 1)
