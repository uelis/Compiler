use ident::Ident;
use std::collections::HashMap;
use std::fmt;

#[derive(Debug)]
pub struct NamingContext {
    idents: HashMap<String, Ident>,
    names: HashMap<Ident, String>,
}

impl<'a> NamingContext {
    pub fn new() -> NamingContext {
        NamingContext {
            idents: HashMap::new(),
            names: HashMap::new(),
        }
    }

    pub fn ident_of_name(&mut self, name: &str) -> Ident {
        match self.idents.get(name).cloned() {
            Some(i) => i,
            None => {
                let i = Ident::new();
                self.idents.insert(name.to_owned(), i);
                self.names.insert(i, name.to_owned());
                i
            }
        }
    }

    pub fn display(&self, fmt: &mut fmt::Formatter, id: Ident) -> Result<(), fmt::Error> {
        match self.names.get(&id) {
            None => write!(fmt, "{}", id),
            Some(s) => write!(fmt, "{}", s),
        }
    }
}
