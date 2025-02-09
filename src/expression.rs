use super::function::Function;
use std::fmt;

#[derive(Clone, Copy, Eq, PartialEq, Hash)]
pub enum Expression<const N: u32> {
    Constant(Function<N>),
    And(Function<N>, Function<N>),
    Or(Function<N>, Function<N>),
    Xor(Function<N>, Function<N>),
    ButNot(Function<N>, Function<N>),
    NotBut(Function<N>, Function<N>),
}

impl<const N: u32> Expression<N> {
    pub fn evaluate(&self) -> Function<N> {
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

impl<const N: u32> fmt::Display for Expression<N> {
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

impl<const N: u32> fmt::Debug for Expression<N> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self)
    }
}
