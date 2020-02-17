use backend::Platform;
use ident::*;
use intermediate::tree;
use intermediate::tree::*;
use minijava::ast;
use minijava::symbols::{ClassInfo, MethodInfo, SymbolTable};
use naming_context::NamingContext;
use std::marker::PhantomData;

pub struct Translator<'a, P: Platform> {
    symbols: &'a SymbolTable<'a>,
    current_class: Option<&'a ClassInfo<'a>>,
    current_method: Option<&'a MethodInfo<'a>>,
    names: NamingContext,
    phantom: PhantomData<P>,
}

fn bin_op(o: &ast::Binop) -> Result<BinOp, ()> {
    match *o {
        ast::Binop::Add => Ok(BinOp::Plus),
        ast::Binop::Sub => Ok(BinOp::Minus),
        ast::Binop::Mul => Ok(BinOp::Mul),
        ast::Binop::Div => Ok(BinOp::Div),
        ast::Binop::Lt => Err(()),
        ast::Binop::StrictAnd => Err(()),
    }
}

impl<'a, P: Platform> Translator<'a, P> {
    pub fn new(st: &'a SymbolTable) -> Self {
        Translator {
            symbols: st,
            current_class: None,
            current_method: None,
            names: NamingContext::new(),
            phantom: PhantomData,
        }
    }

    fn method_name(&self, classname: &str, method: &str) -> String {
        format!("L{}${}", classname, method)
    }

    fn raise_block_name(&self) -> String {
        let ci = self.current_class.unwrap();
        let mi = self.current_method.unwrap();
        format!("L{}${}$raise", ci.name, mi.name)
    }

    fn new_class(&mut self, class_name: &str) -> Exp {
        let alloc = Label::named("L_halloc");
        match self.symbols.classes.get(&class_name) {
            None => panic!(format!("Internal: unknown class {}", class_name)),
            Some(class_info) => {
                let size = 1 + class_info.fields.len();
                call!(
                    Exp::Name(alloc),
                    Exp::Const(size as i32 * (P::word_size()) as i32)
                )
            }
        }
    }

    fn new_array(&mut self, el: Exp) -> Exp {
        let alloc = Label::named("L_halloc");
        let len = Ident::new();
        let t = Ident::new();
        let len1 = binop!(BinOp::Plus, Exp::Temp(len), Exp::Const(1));
        let size = binop!(BinOp::Mul, len1, Exp::Const(P::word_size() as i32));
        Exp::ESeq(
            vec![
                Stm::Move(Exp::Temp(len), el),
                Stm::Move(Exp::Temp(t), call!(Exp::Name(alloc), size)),
                Stm::Move(Exp::Mem(Box::new(Exp::Temp(t))), Exp::Temp(len)),
            ],
            Box::new(Exp::Temp(t)),
        )
    }

    fn this_address(&self) -> Exp {
        Exp::Param(0)
    }

    fn param_index(&self, x: &str) -> u32 {
        match self.current_method.unwrap().parameters.position(&x) {
            None => panic!(format!("Internal: parameter {} not found", x)),
            Some(i) => i as u32 + 1,
        }
    }

    fn field_address(&self, ethis: Exp, f: &str) -> Exp {
        match self.current_class.unwrap().fields.position(&f) {
            None => panic!(format!("Internal: field {} not found", f)),
            Some(i) => {
                let pos = (i as i32 + 1) * (P::word_size() as i32);
                binop!(BinOp::Plus, ethis, Exp::Const(pos))
            }
        }
    }

    fn array_addr(&self, ea: Exp, ei: Exp) -> Exp {
        use self::tree::BinOp::*;
        use self::tree::Exp::*;
        binop!(
            Plus,
            ea,
            binop!(
                Mul,
                binop!(Plus, ei, Const(1)),
                Const(P::word_size() as i32)
            )
        )
    }

    fn array_addr_const(&self, ea: Exp, i: u32) -> Exp {
        use self::tree::BinOp::*;
        use self::tree::Exp::*;
        binop!(Plus, ea, Const(((i + 1) * P::word_size() as u32) as i32))
    }

    fn array_deref(&mut self, ea: Exp, ei: Exp, raise: Label) -> (Vec<Stm>, Exp) {
        let ok = Label::new();
        match ei {
            Exp::Const(index) if index < 0 => (vec![jump!(raise)], Exp::Temp(Ident::new())),
            Exp::Const(index) => {
                let ta = Ident::new();
                let stms = vec![
                    Stm::Move(Exp::Temp(ta), ea),
                    Stm::CJump(
                        RelOp::GE,
                        Exp::Const(index),
                        self.array_length(Exp::Temp(ta)),
                        raise,
                        ok,
                    ),
                    Stm::Label(ok),
                ];
                (
                    stms,
                    Exp::Mem(Box::new(self.array_addr_const(Exp::Temp(ta), index as u32))),
                )
            }
            _ => {
                let ta = Ident::new();
                let ti = Ident::new();
                let check_lower = Label::new();
                let stms = vec![
                    Stm::Move(Exp::Temp(ta), ea),
                    Stm::Move(Exp::Temp(ti), ei),
                    Stm::CJump(
                        RelOp::GE,
                        Exp::Temp(ti),
                        self.array_length(Exp::Temp(ta)),
                        raise,
                        check_lower,
                    ),
                    Stm::Label(check_lower),
                    Stm::CJump(RelOp::LT, Exp::Temp(ti), Exp::Const(0), raise, ok),
                    Stm::Label(ok),
                ];
                (
                    stms,
                    Exp::Mem(Box::new(self.array_addr(Exp::Temp(ta), Exp::Temp(ti)))),
                )
            }
        }
    }

    fn array_length(&self, ea: Exp) -> Exp {
        Exp::Mem(Box::new(ea))
    }

    fn array_get(&mut self, ea: Exp, ei: Exp) -> Exp {
        let raise_name = self.raise_block_name();
        let raise = Label::named(&raise_name);
        let (stm, exp) = self.array_deref(ea, ei, raise);
        Exp::ESeq(stm, Box::new(exp))
    }

    fn array_put(&mut self, ea: Exp, ei: Exp, ed: Exp) -> Stm {
        let raise_name = self.raise_block_name();
        let raise = Label::named(&raise_name);
        let (mut stm, exp) = self.array_deref(ea, ei, raise);
        stm.push(Stm::Move(exp, ed));
        Stm::Seq(stm)
    }

    fn append_raise_block(&mut self, stm: Stm) -> Stm {
        let raise_name = self.raise_block_name();
        let raise = Label::named(&raise_name);
        let raise_fun = Label::named("L_raise");
        let end = Label::new();
        Stm::Seq(vec![
            stm,
            jump!(end),
            Stm::Label(raise),
            Stm::Move(
                Exp::Temp(Ident::new()),
                call!(Exp::Name(raise_fun), Exp::Const(-1)),
            ),
            Stm::Label(end),
        ])
    }

    fn var_lexp(&mut self, i: &str) -> Exp {
        let ci = self.current_class.unwrap();
        let mi = self.current_method.unwrap();

        if mi.locals.keys().contains(&i) {
            Exp::Temp(self.names.ident_of_name(i))
        } else if mi.parameters.keys().contains(&i) {
            Exp::Param(self.param_index(i))
        } else if ci.fields.keys().contains(&i) {
            Exp::Mem(Box::new(self.field_address(self.this_address(), i)))
        } else {
            panic!(format!("Internal: Variable {} not defined.", i))
        }
    }

    fn cond(&mut self, c: &ast::Exp, lt: Label, lf: Label) -> Stm {
        match *c {
            ast::Exp::Id(_) => {
                let t1 = self.exp(c);
                Stm::CJump(RelOp::NE, t1, Exp::Const(0), lt, lf)
            }
            ast::Exp::Number(_) => panic!("not a boolean"),
            ast::Exp::Op(ref e1, op, ref e2) => match op {
                ast::Binop::StrictAnd => {
                    let li = Label::new();
                    let t1 = self.cond(e1, li, lf);
                    let t2 = self.cond(e2, lt, lf);
                    Stm::Seq(vec![t1, Stm::Label(li), t2])
                }
                ast::Binop::Lt => {
                    let t1 = self.exp(e1);
                    let t2 = self.exp(e2);
                    Stm::CJump(RelOp::GE, t1, t2, lf, lt)
                }
                _ => panic!("not a boolean"),
            },
            ast::Exp::Invoke(_, _, _, _) => {
                let t1 = self.exp(c);
                Stm::CJump(RelOp::NE, t1, Exp::Const(0), lt, lf)
            }
            ast::Exp::Read() => {
                let t1 = self.exp(c);
                Stm::CJump(RelOp::NE, t1, Exp::Const(0), lt, lf)
            }
            ast::Exp::True => jump!(lt),
            ast::Exp::False => jump!(lf),
            ast::Exp::This => panic!("not a boolean"),
            ast::Exp::ArrayGet(_, _) => {
                let t1 = self.exp(c);
                Stm::CJump(RelOp::NE, t1, Exp::Const(0), lt, lf)
            }
            ast::Exp::ArrayLength(_) => panic!("not a boolean"),
            ast::Exp::New(_) => panic!("not a boolean"),
            ast::Exp::NewIntArray(_) => panic!("not a boolean"),
            ast::Exp::Neg(ref e) => self.cond(e, lf, lt),
        }
    }

    fn exp(&mut self, e: &ast::Exp) -> Exp {
        match *e {
            ast::Exp::Id(ref i) => self.var_lexp(i),
            ast::Exp::Number(n) => Exp::Const(n),
            ast::Exp::Op(ref e1, op, ref e2) => match bin_op(&op) {
                Ok(o) => {
                    let t1 = self.exp(e1);
                    let t2 = self.exp(e2);
                    Exp::BinOp(o, Box::new(t1), Box::new(t2))
                }
                Err(()) => {
                    let i = Ident::new();
                    let l1 = Label::new();
                    let l2 = Label::new();
                    Exp::ESeq(
                        vec![
                            Stm::Move(Exp::Temp(i), Exp::Const(0)),
                            self.cond(e, l1, l2),
                            Stm::Label(l1),
                            Stm::Move(Exp::Temp(i), Exp::Const(1)),
                            Stm::Label(l2),
                        ],
                        Box::new(Exp::Temp(i)),
                    )
                }
            },
            ast::Exp::Invoke(ref class_id, ref ef, m, ref args) => {
                let tf = self.exp(ef);
                let mut targs = Vec::new();
                targs.push(tf);
                for arg in args {
                    let e = self.exp(arg);
                    targs.push(e)
                }
                match class_id.get() {
                    None => panic!(format!("Internal: type checker did not fill in class id.")),
                    Some(id) => {
                        let name = self.method_name(self.symbols.class_name_of_id(id).unwrap(), m);
                        let addr = Label::named(&name);
                        Exp::Call(Box::new(Exp::Name(addr)), targs)
                    }
                }
            }
            ast::Exp::True => Exp::Const(1),
            ast::Exp::False => Exp::Const(0),
            ast::Exp::This => self.this_address(),
            ast::Exp::ArrayGet(ref e1, ref e2) => {
                let t1 = self.exp(e1);
                let t2 = self.exp(e2);
                self.array_get(t1, t2)
            }
            ast::Exp::ArrayLength(ref e1) => {
                let t1 = self.exp(e1);
                self.array_length(t1)
            }
            ast::Exp::New(c) => self.new_class(c),
            ast::Exp::NewIntArray(ref es) => {
                let ts = self.exp(es);
                self.new_array(ts)
            }
            ast::Exp::Neg(ref e) => binop!(BinOp::Minus, Exp::Const(1), self.exp(e)),
            ast::Exp::Read() => {
                let read_char = Label::named("L_read");
                Exp::Call(Box::new(Exp::Name(read_char)), vec![])
            }
        }
    }

    fn stm(&mut self, s: &ast::Stm) -> Stm {
        match *s {
            ast::Stm::Assignment(ref x, ref e) => {
                let le = self.var_lexp(x);
                let t = self.exp(e);
                Stm::Move(le, t)
            }
            ast::Stm::ArrayAssignment(ref x, ref ei, ref ev) => {
                let ti = self.exp(ei);
                let tv = self.exp(ev);
                let tx = self.var_lexp(x);
                self.array_put(tx, ti, tv)
            }
            ast::Stm::If(ref ec, ref stt, ref sff) => {
                let lfalse = Label::new();
                let ltrue = Label::new();
                let tc = self.cond(ec, ltrue, lfalse);
                let tst = self.stm(stt);
                let tsf = self.stm(sff);
                let lend = Label::new();
                Stm::Seq(vec![
                    tc,
                    Stm::Label(ltrue),
                    tst,
                    jump!(lend),
                    Stm::Label(lfalse),
                    tsf,
                    Stm::Label(lend),
                ])
            }
            ast::Stm::While(ref ec, ref sbody) => {
                let lloop = Label::new();
                let lbody = Label::new();
                let lend = Label::new();
                let tc = self.cond(ec, lbody, lend);
                let tbody = self.stm(sbody);
                Stm::Seq(vec![
                    Stm::Label(lloop),
                    tc,
                    Stm::Label(lbody),
                    tbody,
                    jump!(lloop),
                    Stm::Label(lend),
                ])
            }
            ast::Stm::Write(ref e) => {
                let t = self.exp(e);
                let print_char = Label::named("L_write");
                let x = Ident::new();
                Stm::Move(Exp::Temp(x), call!(Exp::Name(print_char), t))
            }
            ast::Stm::Println(ref e) => {
                let t = self.exp(e);
                let print_int = Label::named("L_println_int");
                let x = Ident::new();
                Stm::Move(Exp::Temp(x), call!(Exp::Name(print_int), t))
            }
            ast::Stm::Seq(ref ss) => {
                let mut ts = Vec::new();
                for s in ss {
                    ts.push(self.stm(&s))
                }
                Stm::Seq(ts)
            }
        }
    }

    fn method(&mut self, cd: &'a ast::ClassDecl, md: &'a ast::MethodDecl) -> Function {
        // must be ok after type checking
        self.current_class = self.symbols.classes.get(&cd.name);
        self.current_method = self.current_class.unwrap().methods.get(&md.name);

        let mut tbody = self.stm(&md.body);
        tbody = self.append_raise_block(tbody);
        let tret = self.exp(&md.ret);
        let t = Ident::new();
        Function {
            name: Label::named(&self.method_name(cd.name, md.name)),
            nparams: 1 /* this */ + self.current_method.unwrap().parameters.len(),
            body: vec![tbody, Stm::Move(Exp::Temp(t), tret)],
            ret: t,
        }
    }

    fn main(&mut self, prg: &'a ast::Prg) -> Function {
        // must be ok after type checking
        self.current_class = self.symbols.classes.get(&prg.main_class);
        self.current_method = self.current_class.unwrap().methods.get(&"main");

        let tbody = self.stm(&prg.main_body);
        let t = Ident::new();
        Function {
            name: Label::named("Lmain"),
            nparams: 1,
            body: vec![tbody, Stm::Move(Exp::Temp(t), Exp::Const(0))],
            ret: t,
        }
    }

    pub fn process(mut self, prg: &'a ast::Prg) -> Prg {
        let mut functions = vec![];
        for cd in &prg.classes {
            for md in &cd.methods {
                let tmethod = self.method(cd, md);
                functions.push(tmethod)
            }
        }
        let tmain = self.main(prg);
        functions.push(tmain);
        tree::Prg {
            names: self.names,
            functions: functions,
        }
    }
}
