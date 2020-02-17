pub mod ast;
mod lexer;
pub mod parser;
pub mod symbols;
pub mod typecheck;

lalrpop_mod!(pub lalrparser); // synthesized by LALRPOP
