use std::collections::HashMap;
use std::fmt;
use std::ops::{BitAnd, BitOr, BitXor, Not};

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
struct Function(u32);

impl From<Function> for usize {
    fn from(f: Function) -> Self {
        f.0 as usize
    }
}

impl Not for Function {
    type Output = Self;

    fn not(self) -> Self::Output {
        Function(!self.0)
    }
}

impl BitXor for Function {
    type Output = Self;

    fn bitxor(self, other: Self) -> Self::Output {
        Function(self.0 ^ other.0)
    }
}

impl BitAnd for Function {
    type Output = Self;

    fn bitand(self, other: Self) -> Self::Output {
        Function(self.0 & other.0)
    }
}

impl BitOr for Function {
    type Output = Self;

    fn bitor(self, other: Self) -> Self::Output {
        Function(self.0 | other.0)
    }
}

impl fmt::Display for Function {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:016b}", self.0)
    }
}

impl fmt::Debug for Function {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self)
    }
}

#[derive(Clone)]
enum Expression {
    Constant(Function),
    And(Function, Function),
    Or(Function, Function),
    Xor(Function, Function),
    ButNot(Function, Function),
    NotBut(Function, Function),
}

impl Expression {
    fn evaluate(&self) -> Function {
        match self {
            Expression::Constant(f) => *f,
            Expression::And(f1, f2) => (*f1 & *f2) & TAUTOLOGY,
            Expression::Or(f1, f2) => (*f1 | *f2) & TAUTOLOGY,
            Expression::Xor(f1, f2) => (*f1 ^ *f2) & TAUTOLOGY,
            Expression::ButNot(f1, f2) => (*f1 & (!*f2)) & TAUTOLOGY,
            Expression::NotBut(f1, f2) => ((!*f1) & *f2) & TAUTOLOGY,
        }
    }
}

impl fmt::Display for Expression {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Expression::Constant(f1) => write!(f, "{}", f1),
            Expression::And(f1, f2) => write!(f, "({} & {})", f1, f2),
            Expression::Or(f1, f2) => write!(f, "({} | {})", f1, f2),
            Expression::Xor(f1, f2) => write!(f, "({} ^ {})", f1, f2),
            Expression::ButNot(f1, f2) => write!(f, "({} > {})", f1, f2),
            Expression::NotBut(f1, f2) => write!(f, "({} < {})", f1, f2),
        }
    }
}

impl fmt::Debug for Expression {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self)
    }
}

#[derive(Debug)]
struct Result {
    lengths: Vec<u32>,
    expressions: Vec<Vec<Expression>>,
    lists: Vec<Vec<Function>>,
    stats: Vec<u32>,
}

const N: u32 = 4;
const INFINITY: u32 = 0xffffffff;
const TAUTOLOGY: Function = Function(2u32.pow(2u32.pow(N)) - 1);

fn find_normal_lengths(
    inputs: &HashMap<Function, Vec<Expression>>,
    target_functions: &Vec<Function>,
) -> Result {
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
    // initialize c, the number of functions where L(f) = infinity
    let mut c = 2u32.pow(2u32.pow(N) - 1) - N - 1;

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
            // L4. Loop over all functions g and h in lists j and k
            for gi in 0..result.lists[j as usize].len() {
                let start_hi = if j == k { gi + 1 } else { 0 };
                for hi in start_hi..result.lists[k as usize].len() {
                    let g = result.lists[j as usize][gi];
                    let h = result.lists[k as usize][hi];
                    // L5. Loop over all new functions f
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
                                for f in target_functions {
                                    println!("{}: {}", *f, result.lengths[usize::from(*f)]);
                                }
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

fn main() {
    let target_functions: Vec<Function> = vec![
        Function(!0b1011011111100011) & TAUTOLOGY,
        Function(!0b1111100111100100) & TAUTOLOGY,
        Function(!0b1101111111110100) & TAUTOLOGY,
        Function(!0b1011011011011110) & TAUTOLOGY,
        Function(!0b1010001010111111) & TAUTOLOGY,
        Function(!0b1000111111110011) & TAUTOLOGY,
        Function(0b0011111011111111) & TAUTOLOGY,
    ];
    let mut inputs: HashMap<Function, Vec<Expression>> = HashMap::new();
    for k in 1..=N {
        let slice = 2u32.pow(2u32.pow(N - k)) + 1;
        let f = Function(TAUTOLOGY.0 / slice);
        inputs.insert(f, vec![Expression::Constant(f)]);
    }

    let result = find_normal_lengths(&inputs, &target_functions);
    println!("{:?}", result.stats);

    for f in target_functions {
        println!("{}: {:?}", f, result.expressions[usize::from(f)]);
    }
}
