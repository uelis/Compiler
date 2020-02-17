extern crate term;

use std::cmp::{max, min};
use std::fmt::{Display, Error, Formatter};

#[derive(Debug)]
pub struct ErrorContext {
    lines: Vec<(String, usize, usize)>,
    line: usize,
    column: usize,
}

#[derive(Debug)]
pub struct CompileError {
    pub context: Option<ErrorContext>,
    pub msg: String,
}

impl ErrorContext {
    pub fn new(source: &str, start: usize, end: usize) -> Option<ErrorContext> {
        let mut chars = source.char_indices();

        let mut start_line = 1;
        let mut start_col = 1;
        while let Some((p, c)) = chars.next() {
            if p >= start {
                break;
            };
            if c == '\n' {
                start_line += start_line;
                start_col = 1;
            } else {
                start_col += start_col;
            }
        }

        chars = source.char_indices();
        let mut current_line_start = 0;
        let mut lines = vec![];
        for (p, c) in chars {
            if c == '\n' {
                if p >= start {
                    let l = (&source[current_line_start..p]).to_owned();
                    let s = max(start, current_line_start) - current_line_start;
                    let e = min(p, end) - current_line_start;
                    lines.push((l, s, e));
                };
                if p >= end {
                    return Some(ErrorContext {
                        lines: lines,
                        line: start_line,
                        column: start_col,
                    });
                }
                current_line_start = p + 1;
            }
        }
        None
    }

    pub fn report(self: &ErrorContext) -> Result<(), ::std::io::Error> {
        let mut term = term::stderr().unwrap(); // TODO
        writeln!(term, "In line {}, column {}:", self.line, self.column)?;
        term.fg(term::color::RED)?;
        writeln!(term, "|")?;
        for &(ref l, s, e) in &self.lines {
            write!(term, "| ")?;
            term.reset()?;
            writeln!(term, "{}", l)?;
            term.fg(term::color::RED)?;
            writeln!(term, "| {}{}", " ".repeat(s), "^".repeat(e - s))?;
            term.reset()?
        }
        Ok(())
    }
}

impl CompileError {
    pub fn new(msg: &str) -> CompileError {
        CompileError {
            msg: msg.to_owned(),
            context: None,
        }
    }

    pub fn new_with_context(source: &str, start: usize, end: usize, msg: &str) -> CompileError {
        CompileError {
            msg: msg.to_owned(),
            context: ErrorContext::new(source, start, end),
        }
    }

    pub fn report(self: &CompileError) -> Result<(), ::std::io::Error> {
        let mut term = term::stderr().unwrap(); // TODO
        writeln!(term)?;
        writeln!(term, "Error: {}", self.msg)?;
        match self.context {
            None => (),
            Some(ref ctx) => ctx.report()?,
        };
        writeln!(term)
    }
}

impl Display for CompileError {
    fn fmt(&self, fmt: &mut Formatter) -> Result<(), Error> {
        write!(fmt, "{}", self.msg)
    }
}
