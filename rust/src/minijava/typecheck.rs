use error::CompileError;
use minijava::ast::*;
use minijava::symbols::{ClassInfo, MethodInfo, SymbolTable};
use std::ops::Deref;

pub fn verify_prg(st: &SymbolTable, prg: &Prg) -> Result<(), CompileError> {
    // special case for main method
    let main_class_info = st.classes.get(&prg.main_class).unwrap();
    let main_method_info = main_class_info.methods.get(&"main").unwrap();
    verify_stm(st, main_class_info, main_method_info, &prg.main_body)?;

    for cd in &prg.classes {
        verify_class(st, cd)?
    }
    Ok(())
}

fn verify_class(st: &SymbolTable, cd: &ClassDecl) -> Result<(), CompileError> {
    let ci = st.classes.get(&cd.name).unwrap(); // all classes must be in symbol table
    for md in &cd.methods {
        verify_method(st, ci, md)?
    }
    Ok(())
}

fn verify_method(st: &SymbolTable, ci: &ClassInfo, md: &MethodDecl) -> Result<(), CompileError> {
    for &(ref x, _) in &md.locals {
        for &(ref y, _) in &md.parameters {
            if x == y {
                let msg = format!(
                    "Local variable {} may not shadow parameter in method {}.",
                    &x, &md.name
                );
                return Err(CompileError::new(&msg));
            }
        }
    }
    let mi = ci.methods.get(&md.name).unwrap(); // must be in symbol table
    verify_stm(st, ci, mi, &md.body)?;
    let t = infer_type(st, ci, mi, &md.ret)?;
    if t == md.ret_type {
        Ok(())
    } else {
        let msg = format!(
            "Return expression in method {} has type {:?}, but {:?} is expected.",
            &md.name, &t, &md.ret_type
        );
        Err(CompileError::new(&msg))
    }
}

fn verify_stm(
    st: &SymbolTable,
    ci: &ClassInfo,
    mi: &MethodInfo,
    s: &Stm,
) -> Result<(), CompileError> {
    use minijava::ast::Stm::*;
    match *s {
        Assignment(ref x, ref e) => {
            let expected = lookup_var(ci, mi, x)?;
            let actual = infer_type(st, ci, mi, e)?;
            if *expected == actual {
                Ok(())
            } else {
                let msg = format!(
                    "Variable {} has type {:?}, but is assigned with type {:?}.",
                    &x, &actual, &expected
                );
                Err(CompileError::new(&msg))
            }
        }
        ArrayAssignment(ref x, ref i, ref e) => {
            let expected = lookup_var(ci, mi, x)?;
            let idx = infer_type(st, ci, mi, i)?;
            let actual = infer_type(st, ci, mi, e)?;
            if idx != Type::Int {
                return Err(CompileError::new(&format!(
                    "Array index must be of type int."
                )));
            };
            match *expected {
                Type::Arr(ref t) => {
                    if *t.deref() != actual {
                        Err(CompileError::new(&format!("Array types do not match.")))
                    } else {
                        Ok(())
                    }
                }
                _ => Err(CompileError::new(&format!("Array type expected."))),
            }
        }
        If(ref e, ref s1, ref s2) => {
            let t = infer_type(st, ci, mi, &e)?;
            if t != Type::Bool {
                Err(CompileError::new(&format!(
                    "Condition in if must have type bool."
                )))
            } else {
                verify_stm(st, ci, mi, s1)?;
                verify_stm(st, ci, mi, s2)?;
                Ok(())
            }
        }
        While(ref e, ref s) => {
            let t = infer_type(st, ci, mi, &e)?;
            if t != Type::Bool {
                Err(CompileError::new(&format!(
                    "Condition in while must have type bool."
                )))
            } else {
                verify_stm(st, ci, mi, s)?;
                Ok(())
            }
        }
        Write(ref e) => {
            let t = infer_type(st, ci, mi, &e)?;
            if t != Type::Int {
                Err(CompileError::new(&format!(
                    "print((char)i) takes and int, not {:?}.",
                    t
                )))
            } else {
                Ok(())
            }
        }
        Println(ref e) => {
            let t = infer_type(st, ci, mi, &e)?;
            if t != Type::Int {
                Err(CompileError::new(&format!(
                    "print(i) takes and int, not {:?}.",
                    t
                )))
            } else {
                Ok(())
            }
        }
        Seq(ref stms) => {
            for s in stms.iter() {
                verify_stm(st, ci, mi, s)?
            }
            Ok(())
        }
    }
}

fn infer_type<'a>(
    st: &SymbolTable<'a>,
    ci: &ClassInfo<'a>,
    mi: &MethodInfo<'a>,
    e: &Exp<'a>,
) -> Result<Type<'a>, CompileError> {
    use minijava::ast::Exp::*;
    use minijava::ast::Type::*;
    match *e {
        Id(ref x) => lookup_var(ci, mi, x).map(|t| t.to_owned()),
        Number(_) => Ok(Int),
        Op(ref l, op, ref r) => {
            let tl = infer_type(st, ci, mi, l)?;
            let tr = infer_type(st, ci, mi, r)?;
            let tops = match op {
                Binop::StrictAnd => Type::Bool,
                Binop::Add | Binop::Sub | Binop::Mul | Binop::Div | Binop::Lt => Type::Int,
            };
            let tres = match op {
                Binop::StrictAnd | Binop::Lt => Type::Bool,
                Binop::Add | Binop::Sub | Binop::Mul | Binop::Div => Type::Int,
            };
            if tl != tops || tr != tops {
                let msg = format!(
                    "Both operands of {:?} are expected to have type {:?}.",
                    op, tops
                );
                Err(CompileError::new(&msg))
            } else {
                Ok(tres)
            }
        }
        Invoke(ref c, ref e, ref m, ref args) => {
            let class_of_e = match infer_type(st, ci, mi, e)? {
                Class(c) => c,
                _ => return Err(CompileError::new(&format!("Class type expected {:?}.", e))),
            };
            // write class id in term
            c.set(st.id_of_class(&class_of_e));
            let eci = st.classes.get(&(&class_of_e[..])).unwrap(); // class must exist
            let emi = eci.methods.get(m).ok_or_else(|| {
                CompileError::new(&format!("Unknown method {}.{}.", &class_of_e, m))
            })?;

            if args.len() < emi.parameters.len() {
                return Err(CompileError::new(&format!("Not enough parameters.")));
            } else if args.len() > emi.parameters.len() {
                return Err(CompileError::new(&format!("Too many parameters.")));
            }

            for (ref a, f) in args.iter().zip(emi.parameters.iter()) {
                let t = infer_type(st, ci, mi, a)?;
                let tf = emi.parameters.get(f).unwrap();
                if t != *tf {
                    let msg = format!(
                        "Parameter does not match: \
                         expected {:?}, actual {:?}.",
                        tf, t
                    );
                    return Err(CompileError::new(&msg));
                }
            }

            Ok(emi.ret_ty.to_owned())
        }
        ArrayGet(ref a, ref i) => {
            match infer_type(st, ci, mi, i)? {
                Int => (),
                _ => {
                    return Err(CompileError::new(&format!(
                        "Array index must have type int."
                    )));
                }
            }
            match infer_type(st, ci, mi, a)? {
                Arr(te) => Ok(*te),
                _ => Err(CompileError::new(&format!("Dereferencing non-array."))),
            }
        }
        ArrayLength(ref e) => match infer_type(st, ci, mi, e)? {
            Arr(_) => Ok(Int),
            _ => Err(CompileError::new(&format!(
                "Only arrays have a length field."
            ))),
        },
        True => Ok(Bool),
        False => Ok(Bool),
        New(ref c) => match st.classes.get(&&c[..]) {
            None => Err(CompileError::new(&format!("Unknown class {} in new.", c))),
            Some(_) => Ok(Class(c.to_owned())),
        },
        NewIntArray(ref e) => match infer_type(st, ci, mi, e)? {
            Int => Ok(Arr(Box::new(Int))),
            _ => Err(CompileError::new(&format!("Array size must be int."))),
        },
        Neg(ref e) => match infer_type(st, ci, mi, e)? {
            Bool => Ok(Bool),
            _ => Err(CompileError::new(&format!(
                "Negation works only on booleans."
            ))),
        },
        Read() => Ok(Int),
        This => Ok(Type::Class(ci.name)),
    }
}

fn lookup_var<'a, 'b>(
    ci: &'b ClassInfo<'a>,
    mi: &'b MethodInfo<'a>,
    x: &'a str,
) -> Result<&'b Type<'a>, CompileError> {
    mi.locals
        .get(&x)
        .or_else(|| mi.parameters.get(&x))
        .or_else(|| {
            if !mi.is_static {
                ci.fields.get(&x)
            } else {
                None
            }
        })
        .ok_or_else(|| CompileError::new(&format!("Variable {} not defined.", &x)))
}
