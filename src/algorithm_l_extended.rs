use std::collections::HashSet;

use super::chain::Chain;
use super::expression::Expression;
use super::function::Function;

const INFINITY: u32 = 0xffffffff;

#[derive(Debug)]
pub struct Result<const N: u32> {
    pub upper_bounds: Vec<u32>,
    pub first_expressions: Vec<Expression<N>>,
    pub footprints: Vec<HashSet<u32>>,
    pub lists: Vec<Vec<Function<N>>>,
    pub stats: Vec<u32>,
}

pub fn find_upper_bounds_and_footprints<const N: u32>(chain: &Chain<N>) -> Result<N> {
    // U1. Initialize.
    let mut result = Result {
        upper_bounds: vec![INFINITY; 2u32.pow(2u32.pow(N) - 1) as usize],
        first_expressions: vec![],
        footprints: vec![HashSet::new(); 2u32.pow(2u32.pow(N) - 1) as usize],
        lists: vec![vec![]; 2],
        stats: vec![0; 2],
    };
    result.upper_bounds[0] = 0;
    result.stats[0] += 1;
    for chain_expression in &chain.expressions {
        result.upper_bounds[usize::from(chain_expression.function)] = 0;
        result.lists[0].push(chain_expression.function);
        result.stats[0] += 1;
    }

    let mut num_removed_infinities = 0;
    for j in 0..chain.expressions.len() {
        for k in j + 1..chain.expressions.len() {
            let g = chain.expressions[j as usize].function;
            let h = chain.expressions[k as usize].function;
            for expr in [
                Expression::And(g, h),
                Expression::Or(g, h),
                Expression::Xor(g, h),
                Expression::ButNot(g, h),
                Expression::NotBut(g, h),
            ] {
                let f = expr.evaluate();
                if chain.function_lookup.contains_key(&f) {
                    continue;
                }
                if result.upper_bounds[usize::from(f)] == INFINITY {
                    num_removed_infinities += 1;
                }
                result.upper_bounds[usize::from(f)] = 1;
                result.lists[1].push(f);
                result.stats[1] += 1;

                result.footprints[usize::from(f)].insert(result.first_expressions.len() as u32);
                result.first_expressions.push(expr);
            }
        }
    }
    // initialize c, the number of functions where U(f) = infinity
    let mut c =
        2u32.pow(2u32.pow(N) - 1) - num_removed_infinities - chain.expressions.len() as u32 - 1;

    // U2. Loop over r = 2, 3, ... while c > 0
    for r in 2.. {
        if c == 0 {
            break;
        }
        while result.lists.len() <= r as usize {
            result.lists.push(vec![]);
        }
        while result.stats.len() <= r as usize {
            result.stats.push(0);
        }
        // U3. Loop over j = [(r-1)/2], ..., 0, k = r - 1 - j
        for j in (0..=(r - 1) / 2).rev() {
            let k = r - 1 - j;
            // U4. Loop over all functions g and h in lists j and k
            for gi in 0..result.lists[j as usize].len() {
                let start_hi = if j == k { gi + 1 } else { 0 };
                for hi in start_hi..result.lists[k as usize].len() {
                    let g = result.lists[j as usize][gi];
                    let h = result.lists[k as usize][hi];

                    let u: u32;
                    let v: HashSet<u32> = result.footprints[usize::from(g)]
                        .union(&result.footprints[usize::from(h)])
                        .cloned()
                        .collect();

                    if (result.footprints[usize::from(g)]
                        .intersection(&result.footprints[usize::from(h)]))
                    .next()
                    .is_none()
                    {
                        u = r;
                    } else {
                        u = r - 1;
                    }

                    // U5. Loop over all new functions f
                    for expr in [
                        Expression::And(g, h),
                        Expression::Or(g, h),
                        Expression::Xor(g, h),
                        Expression::ButNot(g, h),
                        Expression::NotBut(g, h),
                    ] {
                        let f = expr.evaluate();
                        // U6. Update upper_bound(f) and footprint(f)
                        if result.upper_bounds[usize::from(f)] == INFINITY {
                            result.upper_bounds[usize::from(f)] = u;
                            result.footprints[usize::from(f)] = v.clone();
                            result.lists[u as usize].push(f);
                            result.stats[u as usize] += 1;
                            if c == 0 {
                                eprintln!("error: didn't expect c to become negative here");
                            }
                            c -= 1;
                        } else if result.upper_bounds[usize::from(f)] > u {
                            let previous_upper_bound = result.upper_bounds[usize::from(f)];
                            result.lists[previous_upper_bound as usize].retain(|&x| x != f);
                            result.stats[previous_upper_bound as usize] -= 1;
                            result.lists[u as usize].push(f);
                            result.stats[u as usize] += 1;
                            result.upper_bounds[usize::from(f)] = u;
                            result.footprints[usize::from(f)] = v.clone();
                        } else if result.upper_bounds[usize::from(f)] == u {
                            result.footprints[usize::from(f)].extend(&v);
                        }
                    }
                }
            }
        }
    }

    result
}
