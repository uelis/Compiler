use backend::x86::X86Platform;
use backend::Platform;
use error::CompileError;
use std::env;
use std::fmt::Display;
use std::fs::File;
use std::io::{BufWriter, Read, Write};
use std::path::Path;

extern crate hashbrown;
#[macro_use]
extern crate lalrpop_util;
extern crate term;
extern crate unicode_xid;

pub mod backend;
pub mod collections;
pub mod error;
pub mod ident;
pub mod intermediate;
pub mod minijava;
pub mod naming_context;

fn error(error: CompileError, exit: i32) -> ! {
    error.report().unwrap();
    //    println!("{}", msg);
    std::process::exit(exit)
}

fn read_source(source: &str) -> std::io::Result<String> {
    let mut f = File::open(source)?;
    let mut s = String::new();
    f.read_to_string(&mut s)?;
    Ok(s)
}

fn write_result(source: &str, ext: &str, code: &dyn Display) -> std::io::Result<()> {
    let file = match Path::new(source).file_name() {
        None => std::ffi::OsStr::new("a"),
        Some(file) => file,
    };
    let s = Path::new(file).with_extension(ext);
    let file = File::create(s)?;
    let mut f = BufWriter::new(file);
    write!(f, "{}", code)
}

fn compile(file: &str) -> Result<(), error::CompileError> {
    let source = match read_source(&file) {
        Err(e) => {
            return Err(CompileError::new(&format!(
                "Cannot read input: {} ({})",
                file, e
            )));
        }
        Ok(source) => source,
    };

    let ast = minijava::parser::parse_prg(&source)?;

    let symbol_table = minijava::symbols::SymbolTable::new(&ast)?;

    minijava::typecheck::verify_prg(&symbol_table, &ast)?;

    let intermediate = {
        let translator = intermediate::Translator::<X86Platform>::new(&symbol_table);
        translator.process(&ast)
    };

    let canonized = {
        let canonizer = intermediate::Canonizer::new();
        canonizer.process(intermediate)
    };

    let traced = {
        let tracer = intermediate::Tracer::new();
        tracer.process(canonized)
    };

    let mut code = X86Platform::code_gen(&traced);

    backend::regalloc::alloc::<X86Platform>(&mut code);

    if let Err(_) = write_result(file, "s", &code) {
        return Err(CompileError::new("Cannot write file"));
    }
    Ok(())
}

fn main() {
    let mut args = env::args();
    args.next(); // discard executable source

    for argument in args {
        match compile(&argument) {
            Err(e) => error(e, 1),
            Ok(()) => (),
        }
    }
}
