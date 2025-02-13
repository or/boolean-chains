use std::collections::HashMap;

use super::expression::Expression;
use super::functionon::{Functionon, N};

const INFINITY: u32 = 0xffffffff;

#[derive(Debug)]
pub struct Result {
    pub lengths: Vec<u32>,
    pub expressions: Vec<Vec<Expression>>,
    pub lists: Vec<Vec<Function>>,
    pub stats: Vec<u32>,
}

pub fn find_normal_lengths(inputs: &HashMap<Function, Expression>) -> Result {
    // L1. Initialize.
    let mut result = Result {
        lengths: vec![INFINITY; 2u32.pow(2u32.pow(N) - 1) as usize],
        expressions: vec![vec![]; 2u32.pow(2u32.pow(N) - 1) as usize],
        lists: vec![vec![]; 1],
        stats: vec![0; 1],
    };
    result.lengths[0] = 0;
    result.stats[0] += 1;
    for f in inputs.keys() {
        result.lengths[usize::from(*f)] = 0;
        result.lists[0].push(*f);
        result.stats[0] += 1;
    }
    // initialize c, the number of functionons where L(f) = infinity
    let mut c = 2u32.pow(2u32.pow(N) - 1) - inputs.len() as u32 - 1;

    // L2. Loop over r = 1, 2, ...
    for r in 1.. {
        while result.lists.len() <= r as usize {
            result.lists.push(vec![]);
        }
        while result.stats.len() <= r as usize {
            result.stats.push(0);
        }
        // L3. Loop over j = 0, 1, ..., r - 1 k = r - 1 - j while j <= k
        for j in 0..=r - 1 {
            let k = r - 1 - j;
            if k < j {
                break;
            }
            // L4. Loop over all functionons g and h in lists j and k
            for gi in 0..result.lists[j as usize].len() {
                let start_hi = if j == k { gi + 1 } else { 0 };
                for hi in start_hi..result.lists[k as usize].len() {
                    let g = result.lists[j as usize][gi];
                    let h = result.lists[k as usize][hi];
                    // L5. Loop over all new functionons f
                    for expr in [
                        Expression::And(g, h),
                        Expression::Or(g, h),
                        Expression::Xor(g, h),
                        Expression::ButNot(g, h),
                        Expression::NotBut(g, h),
                    ] {
                        let f = expr.evaluate();
                        // L6. Add f if it is new
                        if result.lengths[usize::from(f)] == INFINITY {
                            result.lengths[usize::from(f)] = r;
                            result.expressions[usize::from(f)].push(expr);
                            result.lists[r as usize].push(f);
                            result.stats[r as usize] += 1;
                            c -= 1;
                            if c == 0 {
                                return result;
                            }
                        }
                    }
                }
            }
        }
    }

    result
}
