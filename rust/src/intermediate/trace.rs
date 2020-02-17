use ident::Label;
use intermediate::tree::*;
use std::collections::HashMap;
use std::collections::HashSet;
use std::collections::LinkedList;
use std::mem;

pub struct Tracer;

impl Tracer {
    pub fn new() -> Self {
        Tracer {}
    }

    pub fn process(self, p: Prg) -> Prg {
        trace_prg(p)
    }
}

pub fn trace_prg(p: Prg) -> Prg {
    let mut traced_methods = Vec::new();
    for m in p.functions {
        traced_methods.push(trace_function(m))
    }
    Prg {
        names: p.names,
        functions: traced_methods,
    }
}

pub fn trace_function(m: Function) -> Function {
    let end_label = Label::new();
    let (start_label, blocks) = build_blocks(m.body, end_label);
    let stms = trace(start_label, blocks, end_label);

    Function {
        name: m.name,
        nparams: m.nparams,
        body: stms,
        ret: m.ret,
    }
}

struct Block {
    label: Label,
    stms: Vec<Stm>,
    transfer: Stm,
}

struct BlockBuilder {
    blocks: HashMap<Label, Block>,
    current: Option<(Label, Vec<Stm>)>,
}

impl BlockBuilder {
    fn new() -> BlockBuilder {
        BlockBuilder {
            current: None,
            blocks: HashMap::new(),
        }
    }

    fn start_new(&mut self, l: Label) {
        self.current = Some((l, vec![Stm::Label(l)]));
    }

    fn push_current(&mut self, s: Stm) {
        match self.current {
            None => panic!("assert: push_current"),
            Some((_, ref mut ss)) => ss.push(s),
        }
    }

    fn finish_current(&mut self, transfer: Stm) {
        let current = mem::replace(&mut self.current, None);
        match current {
            None => (),
            Some((l, ss)) => {
                let b = Block {
                    label: l,
                    stms: ss,
                    transfer: transfer,
                };
                self.blocks.insert(l, b);
                self.current = None
            }
        }
    }
}

fn build_blocks(stms: Vec<Stm>, end_label: Label) -> (Label, HashMap<Label, Block>) {
    let mut builder = BlockBuilder::new();

    let start_label = match stms.get(0) {
        Some(&Stm::Label(l)) => l,
        _ => Label::new(),
    };
    builder.start_new(start_label);

    for s in stms {
        match s {
            Stm::Label(l) => {
                builder.finish_current(jump!(l));
                builder.start_new(l)
            }
            Stm::Jump(_, _) => builder.finish_current(s),
            Stm::CJump(_, _, _, _, _) => builder.finish_current(s),
            _ => builder.push_current(s),
        }
    }
    builder.finish_current(jump!(end_label));
    return (start_label, builder.blocks);
}

fn trace(start_label: Label, mut blocks: HashMap<Label, Block>, end_label: Label) -> Vec<Stm> {
    let mut ordered = Vec::new();

    if blocks.is_empty() {
        return ordered;
    }

    let mut added = HashSet::new();
    added.insert(end_label);

    let mut to_trace: LinkedList<Label> = LinkedList::new();

    to_trace.push_back(start_label);
    while let Some(l) = to_trace.pop_front() {
        if !added.contains(&l) {
            let block = blocks.remove(&l).unwrap();
            // remove unneccessary jumps to the next instruction
            match ordered.pop() {
                Some(Stm::Jump(Exp::Name(ref l1), _)) if *l1 == block.label => (),
                Some(s) => ordered.push(s),
                None => (),
            }
            for s in block.stms {
                ordered.push(s);
            }
            match block.transfer {
                Stm::Jump(e, ds) => {
                    for d in &ds {
                        to_trace.push_front(d.clone())
                    }
                    ordered.push(Stm::Jump(e, ds))
                }
                Stm::CJump(r, e1, e2, dt, df) => {
                    if !added.contains(&df) {
                        to_trace.push_front(dt);
                        to_trace.push_front(df);
                        ordered.push(Stm::CJump(r, e1, e2, dt, df))
                    } else if !added.contains(&dt) {
                        to_trace.push_front(df);
                        to_trace.push_front(dt);
                        ordered.push(Stm::CJump(r.neg(), e1, e2, df, dt))
                    } else {
                        let dummy_label = Label::new();
                        ordered.push(Stm::CJump(r, e1, e2, dt, dummy_label));
                        ordered.push(Stm::Label(dummy_label));
                        ordered.push(jump!(df))
                    }
                }
                _ => panic!("trace: assert transfer"),
            }
            added.insert(l.clone());
        }
    }
    ordered.push(Stm::Label(end_label));
    ordered
}
