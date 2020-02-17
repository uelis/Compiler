use backend::graph::Graph;
use backend::Platform;
use backend::{MachineFunction, MachineInstr};
use hashbrown::HashMap;
use std::fmt::{Display, Error, Formatter};

pub struct FlowGraph<'a, P>
where
    P: Platform,
    P::Function: 'a,
{
    pub function: &'a P::Function,
    pub graph: Graph<usize>,
}

impl<'a, P> FlowGraph<'a, P>
where
    P: Platform,
{
    pub fn new(f: &'a P::Function) -> FlowGraph<'a, P> {
        let mut graph = Graph::new();
        let body = f.body();
        let n = body.len();
        let mut targets = HashMap::new();

        for i in 0..n {
            graph.add_node(i);
            body[i].is_label().map(|t| targets.insert(t, i));
        }

        for i in 0..n {
            if i + 1 < n && body[i].is_fall_through() {
                graph.add_edge(i, i + 1)
            }
            for t in body[i].jumps() {
                graph.add_edge(i, targets[&t])
            }
        }
        FlowGraph {
            function: f,
            graph: graph,
        }
    }
}

impl<'a, P> Display for FlowGraph<'a, P>
where
    P: Platform,
    P::Instr: Display,
{
    fn fmt(&self, fmt: &mut Formatter) -> Result<(), Error> {
        self.graph
            .display_dot(fmt, &|i, fmt| write!(fmt, "{}", self.function.body()[*i]))
    }
}
