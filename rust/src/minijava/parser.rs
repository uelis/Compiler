use error::CompileError;
use lalrpop_util::ParseError;
use minijava::ast::Prg;
use minijava::lalrparser::PrgParser;
use minijava::lexer::Error;
use minijava::lexer::ErrorCode::*;
use minijava::lexer::Tokenizer;

extern crate term;

pub fn parse_prg(source: &str) -> Result<Prg, CompileError> {
    let source_tokenizer = Tokenizer::new(source, 0);
    PrgParser::new()
        .parse(source, source_tokenizer)
        .map_err(|e| match e {
            ParseError::InvalidToken { location } => {
                let msg = format!("Parse error: {:?}", e);
                CompileError::new_with_context(source, location, location + 1, &msg)
            }
            ParseError::UnrecognizedToken {
                token: Some((l1, _, l2)),
                expected,
            } => {
                let mut msg = "Invalid token, expected ".to_owned();
                let mut sep = "";
                for tok in expected {
                    msg = format!("{}{}{}", msg, sep, tok);
                    sep = " or ";
                }
                msg = format!("{}.", msg);
                CompileError::new_with_context(source, l1, l2, &msg)
            }
            ParseError::User {
                error: Error { location, code },
            } => {
                let msg = match code {
                    UnrecognizedToken => "Unknown symbol",
                    UnterminatedStringLiteral => "Non-terminated string",
                };
                CompileError::new_with_context(source, location, location + 1, &msg)
            }
            _ => CompileError::new(&format!("Parse error: {:?}", e)),
        })
}
