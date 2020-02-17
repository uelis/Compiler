use backend::flow::FlowGraph;
use backend::Platform;
use backend::{MachineFunction, MachineInstr};
use hashbrown::HashSet;

#[derive(Debug)]
pub struct Liveness<'a, P>
where
    P: Platform,
    P::Function: 'a,
{
    pub function: &'a P::Function,
    pub live_in: Vec<HashSet<P::Reg>>,
    pub live_out: Vec<HashSet<P::Reg>>,
}

impl<'a, P> Liveness<'a, P>
where
    P: Platform,
{
    pub fn new(mut flow: FlowGraph<'a, P>) -> Liveness<'a, P> {
        let body = flow.function.body();
        let n = body.len();
        let mut live_in: Vec<HashSet<P::Reg>> = Vec::with_capacity(n);
        let mut live_out: Vec<HashSet<P::Reg>> = Vec::with_capacity(n);

        for _ in 0..n {
            live_in.push(HashSet::default());
            live_out.push(HashSet::default());
        }

        let mut change = true;
        while change {
            change = false;
            for a in (0..n).rev() {
                let k = live_in[a].len();

                for m in flow.graph.get_successors(a) {
                    for t in live_in[*m].clone() {
                        if live_out[a].insert(t) {
                          change = true;
                          live_in[a].insert(t);
                        }
                    }
                }
                for t in body[a].defs() {
                    live_in[a].remove(&t);
                }
                for t in body[a].uses() {
                    live_in[a].insert(t);
                }
                change |= live_in[a].len() > k;
            }
        }
        Liveness {
            function: flow.function,
            live_in: live_in,
            live_out: live_out,
        }
    }
}
