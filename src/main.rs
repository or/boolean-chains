mod algorithm_l;
mod algorithm_l_extended;
mod expression;
mod function;
use std::collections::HashMap;
use std::collections::HashSet;

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
            if algorithm_result.footprints[usize::from(f)].contains(&(i as u32)) {
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
    let target_functions_set: HashSet<Function<N>> = target_functions.clone().into_iter().collect();
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

    loop {
        println!("inputs:");
        for (f, expr) in &inputs {
            println!("  {} = {}", f, expr);
        }
        let result2 = find_upper_bounds_and_footprints(&inputs);
        println!("{:?}", result2.stats);
        let frequencies = count_first_expressions_in_footprints(&result2, &target_functions);
        let mut range: Vec<usize> = (0..result2.first_expressions.len()).collect();
        range.sort_by_key(|&x| -(frequencies[x] as i32));

        for &f in &target_functions {
            println!("{f}:");
            for &i in &range {
                if result2.footprints[usize::from(f)].contains(&(i as u32)) {
                    println!(
                        "  {}: {}",
                        frequencies[i as usize], result2.first_expressions[i as usize]
                    );
                }
            }
        }

        let expr = result2.first_expressions[range[0]];
        println!("first: {}", expr);
        //break;
        inputs.insert(expr.evaluate(), expr);
        for &expr in &result2.first_expressions {
            let f = expr.evaluate();
            if target_functions_set.contains(&f) {
                inputs.insert(f, expr);
            }
        }

        let mut num_fulfilled_target_functions: u32 = 0;
        for &f in &target_functions {
            if inputs.contains_key(&f) {
                num_fulfilled_target_functions += 1;
            }
        }
        println!(
            "got {} inputs, {} targets fulfilled",
            inputs.len(),
            num_fulfilled_target_functions
        );
        if num_fulfilled_target_functions as usize == target_functions.len() {
            break;
        }
    }
}
