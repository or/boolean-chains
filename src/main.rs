mod algorithm_l;
mod algorithm_l_extended;
mod expression;
mod function;
use std::collections::HashMap;

use algorithm_l::find_normal_lengths;
use algorithm_l_extended::{find_upper_bounds_and_footprints, Result};
use expression::Expression;
use function::Function;

const N: u32 = 4;

fn count_first_expressions_in_footprints<const N: u32>(
    algorithm_result: &Result<N>,
    target_functions: &Vec<Function<N>>,
) -> Vec<u32> {
    let mut result = vec![0; algorithm_result.first_expressions.len() as usize];
    for &f in target_functions {
        for i in 0..algorithm_result.first_expressions.len() {
            if algorithm_result.footprints[usize::from(f)] & (1 << i) > 0 {
                result[i as usize] += 1;
            }
        }
    }

    result
}

fn main() {
    let target_functions: Vec<Function<N>> = vec![
        Function::new(!0b1011011111100011),
        Function::new(!0b1111100111100100),
        Function::new(!0b1101111111110100),
        Function::new(!0b1011011011011110),
        Function::new(!0b1010001010111111),
        Function::new(!0b1000111111110011),
        Function::new(0b0011111011111111),
    ];
    let mut inputs: HashMap<Function<N>, Expression<N>> = HashMap::new();
    for k in 1..=N {
        let slice = 2u32.pow(2u32.pow(N - k)) + 1;
        let f = Function::new(Function::<N>::TAUTOLOGY.0 / slice);
        inputs.insert(f, Expression::Constant(f));
    }

    let result = find_normal_lengths(&inputs);
    for &f in &target_functions {
        println!("{}: {}", f, result.lengths[usize::from(f)]);
    }
    println!("{:?}", result.stats);

    for &f in &target_functions {
        println!("{}: {:?}", f, result.expressions[usize::from(f)]);
    }

    let result2 = find_upper_bounds_and_footprints(&inputs);
    println!("{:?}", result2.stats);
    let frequencies = count_first_expressions_in_footprints(&result2, &target_functions);
    let mut range: Vec<u32> = (0..result2.first_expressions.len() as u32).collect();
    range.sort_by_key(|&x| -(frequencies[x as usize] as i32));

    for &f in &target_functions {
        println!("{f}:");
        for &i in &range {
            if result2.footprints[usize::from(f)] & (1 << i) > 0 {
                println!(
                    "  {}: {}",
                    frequencies[i as usize], result2.first_expressions[i as usize]
                );
            }
        }
    }
}
