use backend;
use intermediate;
use minijava;
use std::fs::{DirBuilder, File};
use std::io::prelude::*;
use std::process::{Command, Stdio};

fn test_translate_file(dir: &str, file_name: &str) {
    let mut file = File::open(format!("{}/{}.java", dir, file_name)).unwrap();
    let mut source = String::new();
    file.read_to_string(&mut source).unwrap();

    let ast = minijava::parser::parse_prg(&source).expect("Parse error");
    let symbol_table = minijava::symbols::SymbolTable::new(&ast).unwrap();
    minijava::typecheck::verify_prg(&symbol_table, &ast).unwrap();
    let translated =
        intermediate::Translator::<backend::x86::X86Platform>::new(&symbol_table).process(&ast);
    let canonized = intermediate::Canonizer::new().process(translated);
    let traced = intermediate::Tracer::new().process(canonized);

    let path = &format!("/tmp/{}", file_name);
    DirBuilder::new()
        .recursive(true)
        .create(path)
        .expect("Cannot create test directory");

    let test_tree = format!("{}/{}.tree", path, file_name);
    let test_c = format!("{}/{}.c", path, file_name);
    let test_exe = format!("{}/{}.exe", path, file_name);

    let mut tree_file = File::create(&test_tree).unwrap();
    let _ = write!(tree_file, "{}", traced);

    let tree2c = Command::new("tree2c")
        .arg(&test_tree)
        .output()
        .expect("failed to execute process");
    let mut c_file = File::create(&test_c).unwrap();
    let _ = c_file.write_all(&tree2c.stdout);
    let _ = c_file.flush();

    let _gcc = Command::new("gcc")
        .arg("-m32")
        .arg("test/runtime.c")
        .arg(test_c)
        .arg("-o")
        .arg(&test_exe)
        .output()
        .expect("failed to execute gcc");

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

    let test = Command::new(&test_exe)
        .stdout(Stdio::piped())
        .output()
        .expect("failed to execute program");

    assert!(!java.status.success() || test.status.success());
    assert!(!test.status.success() || java.status.success());
    if java.status.success() {
        assert_eq!(
            String::from_utf8_lossy(&java.stdout),
            String::from_utf8_lossy(&test.stdout)
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
    Brainfuck,
    BubbleSort,
    Div,
    Effects,
    Euler,
    EulerTest,
    Factorial,
    FactorialMem,
    FibInteger,
    Fib,
    FibL,
    GameOfLife,
    Graph,
    Hanoi,
    LinearSearch,
    LinkedList,
    Mandelbrot,
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
