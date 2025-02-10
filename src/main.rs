mod algorithm_l_extended;
mod chain;
mod expression;
mod function;

use algorithm_l_extended::{find_upper_bounds_and_footprints, Result};
use chain::Chain;
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
    let mut chain = Chain::<N>::new(&vec![
        Function::new(!0b1011011111100011),
        Function::new(!0b1111100111100100),
        Function::new(!0b1101111111110100),
        Function::new(!0b1011011011011110),
        Function::new(!0b1010001010111111),
        Function::new(!0b1000111111110011),
        Function::new(0b0011111011111111),
    ]);

    for k in 1..=N {
        let slice = 2u32.pow(2u32.pow(N - k)) + 1;
        let f = Function::new(Function::<N>::TAUTOLOGY.0 / slice);
        chain.add(Expression::Constant(f));
    }

    loop {
        chain.print();
        let result2 = find_upper_bounds_and_footprints(&chain);
        println!("{:?}", result2.stats);
        let frequencies = count_first_expressions_in_footprints(&result2, &chain.targets);
        let mut range: Vec<usize> = (0..result2.first_expressions.len()).collect();
        range.sort_by_key(|&x| -(frequencies[x] as i32));

        for &f in &chain.targets {
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

        chain.add(expr);
        for &expr in &result2.first_expressions {
            let f = expr.evaluate();
            if chain.target_lookup.contains(&f) && !chain.function_lookup.contains_key(&f) {
                chain.add(expr);
            }
        }

        let mut num_fulfilled_target_functions: u32 = 0;
        for &f in &chain.targets {
            if chain.function_lookup.contains_key(&f) {
                num_fulfilled_target_functions += 1;
            }
        }
        println!(
            "got {} inputs, {} targets fulfilled",
            chain.expressions.len(),
            num_fulfilled_target_functions
        );
        if num_fulfilled_target_functions as usize == chain.targets.len() {
            break;
        }
    }
    chain.print();
}
