use backend::flow::FlowGraph;
use backend::interference::Interference;
use backend::liveness::Liveness;
use backend::*;
use hashbrown::{HashMap, HashSet};
use std::iter::FromIterator;

pub fn alloc<P>(p: &mut P::Prg)
where
    P: Platform,
    P::Reg: 'static,
{
    for f in p.functions_mut() {
        alloc_function::<P>(f)
    }
}

fn alloc_function<P>(m: &mut P::Function)
where
    P: Platform,
    P::Reg: 'static,
{
    let colour_result = color::<P>(m);
    //   println!("color: {:#?}", colour_result);
    if colour_result.spills.is_empty() {
        let sigma = |t| *colour_result.colouring.get(&t).unwrap_or(&t);
        m.rename(&sigma)
    } else {
        m.spill(&colour_result.spills);
        alloc_function::<P>(m)
    }
}

#[derive(Debug)]
struct ColourResult<P>
where
    P: Platform,
    P::Reg: 'static,
{
    colouring: HashMap<P::Reg, P::Reg>,
    spills: Vec<P::Reg>,
}

fn color<P>(function: &P::Function) -> ColourResult<P>
where
    P: Platform,
{
    let flow = FlowGraph::<P>::new(function);
    let liveness = Liveness::<P>::new(flow);
    let interference = Interference::new(function, &liveness);
    let mut graph = interference.graph;

    let mut stack: Vec<P::Reg> = Vec::new();
    let k = P::Reg::general_purpose_registers().len();
    let mut low_degrees: Vec<P::Reg> = Vec::new();
    let mut high_degrees: HashMap<P::Reg, usize> = HashMap::default();

    low_degrees.reserve(graph.nodes().len());
    for t in graph.nodes() {
        if t.is_physical() {
            continue;
        };
        let deg = graph.out_degree(t);
        if deg >= k {
            high_degrees.insert(*t, deg);
        } else {
            low_degrees.push(*t);
        }
    }

    // Simplify and Spill
    while low_degrees.len() + high_degrees.len() > 0 {
        let next_temp = match low_degrees.pop() {
            Some(t) => t,
            None => {
                let mut next_temp = *high_degrees.iter().next().unwrap().0;
                let mut max_degree = 0;
                for (t, deg) in &high_degrees {
                    if *deg > max_degree {
                        next_temp = *t;
                        max_degree = *deg;
                    }
                }
                next_temp
            }
        };

        stack.push(next_temp);
        high_degrees.remove(&next_temp);
        for t in graph.get_successors(next_temp) {
            let d = high_degrees.get_mut(t).map(|i| {
                *i -= 1;
                *i
            });
            match d {
                None => (),
                Some(deg) => {
                    if deg < k {
                        high_degrees.remove(t);
                        low_degrees.push(*t)
                    }
                }
            }
        }
    }

    // Select
    let mut colouring: HashMap<P::Reg, P::Reg> = HashMap::default();
    let n = graph.nodes().len();
    colouring.reserve(n);

    for t in P::Reg::physical_registers() {
        colouring.insert(*t, *t);
    }

    let mut actual_spills = Vec::new();
    let usable_colours =
        HashSet::<&P::Reg>::from_iter(P::Reg::general_purpose_registers().iter());

    while let Some(s) = stack.pop() {
        let mut possible_colours = usable_colours.clone();
        for t in graph.get_successors(s) {
            match colouring.get(t) {
                None => (),
                Some(ce) => {
                    possible_colours.remove(ce);
                }
            }
        }
        if let Some(c) = possible_colours.iter().next() {
            colouring.insert(s, **c);
        } else {
            actual_spills.push(s);
        }
    }

    ColourResult {
        colouring: colouring,
        spills: actual_spills,
    }
}
