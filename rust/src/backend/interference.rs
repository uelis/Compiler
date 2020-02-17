use backend::graph::Graph;
use backend::liveness::Liveness;
use backend::platform::MachineReg;
use backend::MachineFunction;
use backend::MachineInstr;
use backend::Platform;

#[derive(Debug)]
pub struct Interference<P>
where
    P: Platform,
    P::Reg: 'static,
{
    pub graph: Graph<P::Reg>,
}

impl<P> Interference<P>
where
    P: Platform,
{
    pub fn new(function: &P::Function, live: &Liveness<P>) -> Interference<P> {
        let ignore: Vec<P::Reg> = P::Reg::physical_registers()
            .iter()
            .filter(|t| !P::Reg::general_purpose_registers().contains(t))
            .cloned()
            .collect();

        let body = function.body();
        let n = body.len();

        let mut interference = Graph::new();

        for i in 0..n {
            for b in body[i].defs() {
                if ignore.contains(&b) {
                    continue;
                }
                interference.add_node(b);
                for c in &live.live_out[i] {
                    interference.add_node(*c);

                    if b == *c || ignore.contains(c) {
                        continue;
                    };
                    match body[i].is_move_between_temps() {
                        Some((_, src)) if src == *c => continue,
                        _ => (),
                    }
                    interference.add_edge(b, *c);
                    interference.add_edge(*c, b);
                }
            }
        }

        Interference {
            graph: interference,
        }
    }
}
