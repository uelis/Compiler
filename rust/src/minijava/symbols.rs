use collections::OrderedMap;
use error::CompileError;
use minijava::ast::*;
use std::cmp::Eq;
use std::fmt::{Display, Error, Formatter};
use std::hash::Hash;

#[derive(Debug, Copy, Clone)]
pub struct ClassId {
    id: usize,
}

#[derive(Debug)]
pub struct SymbolTable<'a> {
    pub main_class: String,
    pub classes: OrderedMap<&'a str, ClassInfo<'a>>,
}

#[derive(Debug)]
pub struct ClassInfo<'a> {
    pub name: &'a str,
    pub super_class: Option<&'a str>,
    pub fields: OrderedMap<&'a str, Type<'a>>,
    pub methods: OrderedMap<&'a str, MethodInfo<'a>>,
}

#[derive(Debug)]
pub struct MethodInfo<'a> {
    pub is_static: bool,
    pub name: &'a str,
    pub ret_ty: Type<'a>,
    pub parameters: OrderedMap<&'a str, Type<'a>>,
    pub locals: OrderedMap<&'a str, Type<'a>>,
}

impl<'a> MethodInfo<'a> {
    pub fn new(name: &'a str, ty: Type<'a>) -> MethodInfo<'a> {
        MethodInfo {
            is_static: false,
            name: name,
            ret_ty: ty,
            parameters: OrderedMap::new(),
            locals: OrderedMap::new(),
        }
    }

    pub fn new_static(name: &'a str, ty: Type<'a>) -> MethodInfo<'a> {
        MethodInfo {
            is_static: true,
            name: name,
            ret_ty: ty,
            parameters: OrderedMap::new(),
            locals: OrderedMap::new(),
        }
    }
}

impl<'a> ClassInfo<'a> {
    pub fn new(name: &'a str) -> ClassInfo<'a> {
        ClassInfo {
            name: name,
            super_class: None,
            fields: OrderedMap::new(),
            methods: OrderedMap::new(),
        }
    }
}

fn try_update<K: Hash + Eq + ToOwned<Owned = K>, V, E>(
    m: &mut OrderedMap<K, V>,
    k: K,
    v: V,
    err: &dyn (Fn(&K) -> E),
) -> Result<(), E> {
    m.put(k, v).map_err(|x| err(&x))
}

impl<'a> SymbolTable<'a> {
    pub fn new(prg: &'a Prg) -> Result<SymbolTable<'a>, CompileError> {
        let mut st = SymbolTable {
            main_class: prg.main_class.to_owned(),
            classes: OrderedMap::new(),
        };
        let mut mci = ClassInfo::new(prg.main_class);
        let mi = MethodInfo::new_static("main", Type::Void);
        try_update(&mut mci.methods, "main", mi, &|x| {
            CompileError::new(&format!("{} already defined.", x))
        })?;

        try_update(&mut st.classes, mci.name, mci, &|x| {
            CompileError::new(&format!("Class {} already defined.", x))
        })?;

        // filling the table is simple enough to do it here
        for cd in &prg.classes {
            let mut ci = ClassInfo::new(cd.name);
            for ft in &cd.fields {
                let (ref f, ref t) = *ft;
                try_update(&mut ci.fields, f.to_owned(), t.to_owned(), &|x| {
                    CompileError::new(&format!("Field {} already defined.", x))
                })?;
            }

            for md in &cd.methods {
                let mut mi = MethodInfo::new(md.name, md.ret_type.to_owned());
                for pt in &md.parameters {
                    let (ref p, ref t) = *pt;
                    try_update(&mut mi.parameters, p.to_owned(), t.to_owned(), &|x| {
                        CompileError::new(&format!("Parameter {} already defined.", x))
                    })?;
                }
                for lt in &md.locals {
                    let (ref l, ref t) = *lt;
                    try_update(&mut mi.locals, l.to_owned(), t.to_owned(), &|x| {
                        CompileError::new(&format!("Local Variable {} already defined.", x))
                    })?;
                }
                try_update(&mut ci.methods, mi.name, mi, &|x| {
                    CompileError::new(&format!("Method {} already defined.", x))
                })?;
            }
            try_update(&mut st.classes, ci.name, ci, &|x| {
                CompileError::new(&format!("Class {} already defined.", x))
            })?;
        }

        Ok(st)
    }

    pub fn id_of_class(&self, name: &'a str) -> Option<ClassId> {
        self.classes.position(&name).map(|x| ClassId { id: x })
    }

    pub fn class_name_of_id(&self, ci: ClassId) -> Option<&&'a str> {
        self.classes.nth(ci.id)
    }
}

impl Display for ClassId {
    fn fmt(&self, fmt: &mut Formatter) -> Result<(), Error> {
        write!(fmt, "{:?}", self.id)
    }
}
