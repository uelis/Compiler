use backend::x86::X86Platform;
use backend::{regalloc, Platform};
use intermediate;
use minijava;
use std::fs::{DirBuilder, File};
use std::io::prelude::*;
use std::process::Command;

fn test_translate_file(dir: &str, file_name: &str) {
    let mut file = File::open(format!("{}/{}.java", dir, file_name)).unwrap();
    let mut source = String::new();
    file.read_to_string(&mut source).unwrap();

    let ast = minijava::parser::parse_prg(&source).expect("Parse error");
    let symbol_table = minijava::symbols::SymbolTable::new(&ast).unwrap();
    minijava::typecheck::verify_prg(&symbol_table, &ast).unwrap();
    let translated = intermediate::Translator::<X86Platform>::new(&symbol_table).process(&ast);
    let canonized = intermediate::Canonizer::new().process(translated);
    let traced = intermediate::Tracer::new().process(canonized);
    let mut code = X86Platform::code_gen(&traced);
    regalloc::alloc::<X86Platform>(&mut code);

    let path = &format!("/tmp/{}", file_name);
    DirBuilder::new()
        .recursive(true)
        .create(path)
        .expect("Cannot create test directory");

    let test_s = format!("{}/{}.s", path, file_name);

    let mut code_file = File::create(&test_s).unwrap();
    let _ = write!(code_file, "{}", code);

    let sim = Command::new("symb386")
        .arg(&test_s)
        .output()
        .expect("failed to execute process");

    let _cp = Command::new("cp")
        .arg(format!("{}/{}.java", dir, file_name))
        .arg(format!("{}/{}.java", path, file_name))
        .output()
        .expect("failed to execute cp");

    let _javac = Command::new("javac")
        .current_dir(path)
        .arg(format!("{}.java", file_name))
        .output()
        .expect("failed to execute javac");

    let java = Command::new("java")
        .current_dir(path)
        .arg(file_name)
        .output()
        .expect("failed to execute java");

    assert!(!java.status.success() || sim.status.success());
    assert!(!sim.status.success() || java.status.success());
    if java.status.success() {
        assert_eq!(
            String::from_utf8_lossy(&java.stdout),
            String::from_utf8_lossy(&sim.stdout)
        )
    }
}

static TEST_DIR: &'static str = "test";

macro_rules! minijava_tests {
    ($($name:ident,)*) => {
    $(
        #[test]
        #[allow(non_snake_case)]
        fn $name() {
            test_translate_file(TEST_DIR, stringify!($name))
        }
    )*
    }
}

minijava_tests! {
    ArrayAccess,
    ArrayBounds,
    BinarySearch,
    BinaryTree,
    Binding,
//    Brainfuck,
    BubbleSort,
    Div,
    Effects,
//    E,
    EulerTest,
    Factorial,
    FactorialMem,
//    FibInteger,
    Fib,
    FibL,
//    GameOfLife,
    Graph,
//    Hanoi,
    LinearSearch,
    LinkedList,
//    Mandelbrot,
    ManyArgs,
    Newton,
    Primes,
    QuickSort,
    ShortCutAnd,
    Stck,
    Sum,
    TestEq,
    TrivialClass,
    While,
}
