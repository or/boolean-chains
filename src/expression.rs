use super::function::Function;
use std::fmt;

#[derive(Clone, Copy, Eq, PartialEq, Hash)]
pub enum Expression {
    Constant(Function),
    And(Function, Function),
    Or(Function, Function),
    Xor(Function, Function),
    ButNot(Function, Function),
    NotBut(Function, Function),
}

impl Expression {
    pub fn evaluate(&self) -> Function {
        match self {
            Expression::Constant(f) => *f,
            Expression::And(f1, f2) => *f1 & *f2,
            Expression::Or(f1, f2) => *f1 | *f2,
            Expression::Xor(f1, f2) => *f1 ^ *f2,
            Expression::ButNot(f1, f2) => *f1 & (!*f2),
            Expression::NotBut(f1, f2) => (!*f1) & *f2,
        }
    }
}

impl fmt::Display for Expression {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Expression::Constant(f1) => write!(f, "{}", f1),
            Expression::And(f1, f2) => write!(f, "{} & {}", f1, f2),
            Expression::Or(f1, f2) => write!(f, "{} | {}", f1, f2),
            Expression::Xor(f1, f2) => write!(f, "{} ^ {}", f1, f2),
            Expression::ButNot(f1, f2) => write!(f, "{} > {}", f1, f2),
            Expression::NotBut(f1, f2) => write!(f, "{} < {}", f1, f2),
        }
    }
}

impl fmt::Debug for Expression {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self)
    }
}
