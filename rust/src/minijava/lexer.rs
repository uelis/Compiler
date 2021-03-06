//! Adapted version of LALRPOP lexer.
use std::str::CharIndices;
use unicode_xid::UnicodeXID;

use self::ErrorCode::*;
use self::Tok::*;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Error {
    pub location: usize,
    pub code: ErrorCode,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum ErrorCode {
    UnrecognizedToken,
    UnterminatedStringLiteral,
}

fn error<T>(c: ErrorCode, l: usize) -> Result<T, Error> {
    Err(Error {
        location: l,
        code: c,
    })
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Tok<'input> {
    // Keywords;
    Public,
    Static,
    Main,
    Class,
    Extends,
    If,
    Else,
    While,
    Return,
    Length,
    True,
    False,
    This,
    New,
    Throws,
    IOException,

    Void,
    Bool,
    Char,
    Int,
    Write,
    Read,
    Println,

    Bang,
    DoubleAmpersand,
    Comma,
    Semicolon,
    Dot,
    Equals,
    Lt,
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    LeftBracket,
    RightBracket,
    Plus,
    Minus,
    Star,
    Slash,

    String(&'input str),
    Id(&'input str),
    Num(i32),
}

pub struct Tokenizer<'input> {
    text: &'input str,
    chars: CharIndices<'input>,
    lookahead: Option<(usize, char)>,
    shift: usize,
}

pub type Spanned<T> = (usize, T, usize);

const KEYWORDS: &'static [(&'static str, Tok<'static>)] = &[
    ("public", Public),
    ("static", Static),
    ("main", Main),
    ("class", Class),
    ("extends", Extends),
    ("while", While),
    ("if", If),
    ("else", Else),
    ("return", Return),
    ("true", True),
    ("false", False),
    ("this", This),
    ("new", New),
    ("length", Length),
    ("void", Void),
    ("char", Char),
    ("boolean", Bool),
    ("int", Int),
    ("throws", Throws),
];

impl<'input> Tokenizer<'input> {
    pub fn new(text: &'input str, shift: usize) -> Tokenizer<'input> {
        let mut t = Tokenizer {
            text: text,
            chars: text.char_indices(),
            lookahead: None,
            shift: shift,
        };
        t.bump();
        t
    }

    fn next_unshifted(&mut self) -> Option<Result<Spanned<Tok<'input>>, Error>> {
        loop {
            return match self.lookahead {
                Some((idx0, '!')) => {
                    self.bump();
                    Some(Ok((idx0, Bang, idx0 + 1)))
                }
                Some((idx0, '&')) => match self.bump() {
                    Some((idx1, '&')) => {
                        self.bump();
                        Some(Ok((idx0, DoubleAmpersand, idx1 + 1)))
                    }
                    _ => Some(error(UnrecognizedToken, idx0)),
                },
                Some((idx0, ',')) => {
                    self.bump();
                    Some(Ok((idx0, Comma, idx0 + 1)))
                }
                Some((idx0, ';')) => {
                    self.bump();
                    Some(Ok((idx0, Semicolon, idx0 + 1)))
                }
                Some((idx0, '.')) => {
                    self.bump();
                    Some(Ok((idx0, Dot, idx0 + 1)))
                }
                Some((idx0, '=')) => {
                    self.bump();
                    Some(Ok((idx0, Equals, idx0 + 1)))
                }
                Some((idx0, '{')) => {
                    self.bump();
                    Some(Ok((idx0, LeftBrace, idx0 + 1)))
                }
                Some((idx0, '[')) => {
                    self.bump();
                    Some(Ok((idx0, LeftBracket, idx0 + 1)))
                }
                Some((idx0, '(')) => {
                    self.bump();
                    Some(Ok((idx0, LeftParen, idx0 + 1)))
                }
                Some((idx0, '<')) => {
                    self.bump();
                    Some(Ok((idx0, Lt, idx0 + 1)))
                }
                Some((idx0, '+')) => {
                    self.bump();
                    Some(Ok((idx0, Plus, idx0 + 1)))
                }
                Some((idx0, '-')) => {
                    self.bump();
                    Some(Ok((idx0, Minus, idx0 + 1)))
                }
                Some((idx0, '*')) => {
                    self.bump();
                    Some(Ok((idx0, Star, idx0 + 1)))
                }
                Some((idx0, '/')) => match self.bump() {
                    Some((_, '/')) => {
                        self.line_comment();
                        continue;
                    }
                    Some((_, '*')) => {
                        self.comment();
                        continue;
                    }
                    _ => Some(Ok((idx0, Slash, idx0 + 1))),
                },
                Some((idx0, '}')) => {
                    self.bump();
                    Some(Ok((idx0, RightBrace, idx0 + 1)))
                }
                Some((idx0, ']')) => {
                    self.bump();
                    Some(Ok((idx0, RightBracket, idx0 + 1)))
                }
                Some((idx0, ')')) => {
                    self.bump();
                    Some(Ok((idx0, RightParen, idx0 + 1)))
                }
                Some((idx0, '"')) => {
                    self.bump();
                    Some(self.string_literal(idx0))
                }
                Some((idx0, c)) if is_identifier_start(c) => Some(self.identifierish(idx0)),
                Some((idx0, c)) if c.is_numeric() => Some(self.number(idx0)),
                Some((_, c)) if c.is_whitespace() => {
                    self.bump();
                    continue;
                }
                Some((idx, _)) => Some(error(UnrecognizedToken, idx)),
                None => None,
            };
        }
    }

    fn bump(&mut self) -> Option<(usize, char)> {
        self.lookahead = self.chars.next();
        self.lookahead
    }

    fn string_literal(&mut self, idx0: usize) -> Result<Spanned<Tok<'input>>, Error> {
        let mut escape = false;
        let terminate = |c: char| {
            if escape {
                escape = false;
                false
            } else if c == '\\' {
                escape = true;
                false
            } else if c == '"' {
                true
            } else {
                false
            }
        };
        match self.take_until(terminate) {
            Some(idx1) => {
                self.bump(); // consume the '"'
                let text = &self.text[idx0 + 1..idx1]; // do not include the "" in the str
                Ok((idx0, String(text), idx1 + 1))
            }
            None => error(UnterminatedStringLiteral, idx0),
        }
    }

    fn identifierish(&mut self, idx0: usize) -> Result<Spanned<Tok<'input>>, Error> {
        let (start, word, end) = self.word(idx0);

        let tok = KEYWORDS
            .iter()
            .filter(|&&(w, _)| w == word)
            .map(|&(_, ref t)| t.clone())
            .next()
            .unwrap_or_else(|| Id(word));

        let st = (start, tok, end);

        // TODO: This is messy
        if word == "System" {
            match self.peek_extended_keyword() {
                None => Ok(st),
                Some(idx) => {
                    let w = &self.text[end..idx];
                    if w == ".out.println" {
                        // TODO
                        self.bump();
                        let id1 = self.lookahead.unwrap().0;
                        self.word(id1);
                        self.bump();
                        let id2 = self.lookahead.unwrap().0;
                        self.word(id2);
                        Ok((start, Println, idx))
                    } else if w == ".out.write" {
                        self.bump();
                        let id1 = self.lookahead.unwrap().0;
                        self.word(id1);
                        self.bump();
                        let id2 = self.lookahead.unwrap().0;
                        self.word(id2);
                        Ok((start, Write, idx))
                    } else if w == ".in.read" {
                        self.bump();
                        let id1 = self.lookahead.unwrap().0;
                        self.word(id1);
                        self.bump();
                        let id2 = self.lookahead.unwrap().0;
                        self.word(id2);
                        Ok((start, Read, idx))
                    } else {
                        Ok(st)
                    }
                }
            }
        } else if word == "java" {
            match self.peek_extended_keyword() {
                None => Ok(st),
                Some(idx) => {
                    let w = &self.text[end..idx];
                    if w == ".io.IOException" {
                        self.bump();
                        let id1 = self.lookahead.unwrap().0;
                        self.word(id1);
                        self.bump();
                        let id2 = self.lookahead.unwrap().0;
                        self.word(id2);
                        Ok((start, IOException, idx))
                    } else {
                        Ok(st)
                    }
                }
            }
        } else {
            Ok(st)
        }
    }

    fn number(&mut self, idx0: usize) -> Result<Spanned<Tok<'input>>, Error> {
        let (start, word, end) = self.numeric(idx0);

        match word.parse::<i32>() {
            Ok(i) => Ok((start, Num(i), end)),
            Err(_) => Err(Error {
                location: start,
                code: UnrecognizedToken,
            }),
        }
    }

    fn numeric(&mut self, idx0: usize) -> Spanned<&'input str> {
        match self.take_while(|c| c.is_numeric()) {
            Some(end) => (idx0, &self.text[idx0..end], end),
            None => (idx0, &self.text[idx0..], self.text.len()),
        }
    }

    fn word(&mut self, idx0: usize) -> Spanned<&'input str> {
        match self.take_while(is_identifier_continue) {
            Some(end) => (idx0, &self.text[idx0..end], end),
            None => (idx0, &self.text[idx0..], self.text.len()),
        }
    }

    fn line_comment(&mut self) {
        self.take_until(|c| c == '\n');
        self.bump();
    }

    fn comment(&mut self) {
        loop {
            self.take_until(|c| c == '*');
            self.bump();
            match self.lookahead {
                None => return (),
                Some((_, '/')) => {
                    self.bump();
                    return ();
                }
                Some(_) => (),
            }
        }
    }

    fn take_while<F>(&mut self, mut keep_going: F) -> Option<usize>
    where
        F: FnMut(char) -> bool,
    {
        self.take_until(|c| !keep_going(c))
    }

    fn take_until<F>(&mut self, mut terminate: F) -> Option<usize>
    where
        F: FnMut(char) -> bool,
    {
        loop {
            match self.lookahead {
                None => {
                    return None;
                }
                Some((idx1, c)) => {
                    if terminate(c) {
                        return Some(idx1);
                    } else {
                        self.bump();
                    }
                }
            }
        }
    }

    fn peek_extended_keyword(&self) -> Option<usize> {
        let mut chars: CharIndices = self.chars.clone();
        let mut l = self.lookahead;
        loop {
            match l {
                None => return None,
                Some((idx1, c)) => {
                    if !(is_identifier_continue(c) || c == '.') {
                        return Some(idx1);
                    } else {
                        l = chars.next();
                    }
                }
            }
        }
    }
}

impl<'input> Iterator for Tokenizer<'input> {
    type Item = Result<Spanned<Tok<'input>>, Error>;

    fn next(&mut self) -> Option<Result<Spanned<Tok<'input>>, Error>> {
        match self.next_unshifted() {
            None => None,
            Some(Ok((l, t, r))) => Some(Ok((l + self.shift, t, r + self.shift))),
            Some(Err(Error { location, code })) => Some(Err(Error {
                location: location + self.shift,
                code: code,
            })),
        }
    }
}

fn is_identifier_start(c: char) -> bool {
    UnicodeXID::is_xid_start(c)
}

fn is_identifier_continue(c: char) -> bool {
    UnicodeXID::is_xid_continue(c)
}
