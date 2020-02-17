use std::collections::HashMap;
use std::fmt::{Debug, Error, Formatter};
use std::hash::Hash;
use std::slice::Iter;

pub struct OrderedMap<S, T>
where
    S: Hash + Eq + ToOwned<Owned = S>,
{
    keys: Vec<S>,
    map: HashMap<S, T>,
}

type OrderedMapIter<'a, S> = Iter<'a, S>;

impl<S, T> OrderedMap<S, T>
where
    S: Hash + Eq + ToOwned<Owned = S>,
{
    pub fn new() -> OrderedMap<S, T> {
        OrderedMap {
            keys: Vec::new(),
            map: HashMap::new(),
        }
    }

    pub fn keys(&self) -> &Vec<S> {
        &self.keys
    }

    pub fn len(&self) -> usize {
        self.keys.len()
    }

    pub fn iter(&self) -> OrderedMapIter<S> {
        self.keys.iter()
    }

    pub fn get(&self, k: &S) -> Option<&T> {
        self.map.get(k)
    }

    pub fn put(&mut self, k: S, v: T) -> Result<(), S> {
        let k1 = k.to_owned();
        match self.map.insert(k, v) {
            None => {
                self.keys.push(k1);
                return Ok(());
            }
            Some(_) => Err(k1),
        }
    }

    pub fn position(&self, x: &S) -> Option<usize> {
        let mut i = 0;
        for c in self.iter() {
            if x == c {
                return Some(i);
            }
            i += 1
        }
        return None;
    }

    pub fn nth(&self, x: usize) -> Option<&S> {
        self.keys.get(x)
    }
}

impl<T> IntoIterator for OrderedMap<String, T> {
    type Item = String;
    type IntoIter = ::std::vec::IntoIter<String>;

    fn into_iter(self) -> Self::IntoIter {
        self.keys.into_iter()
    }
}

impl<S, T: Debug> Debug for OrderedMap<S, T>
where
    S: Debug + Hash + Eq + ToOwned<Owned = S>,
{
    fn fmt(&self, fmt: &mut Formatter) -> Result<(), Error> {
        for d in self.iter() {
            write!(fmt, "{:#?}", d)?
        }
        write!(fmt, "{:#?}", self.map)?;
        Ok(())
    }
}
