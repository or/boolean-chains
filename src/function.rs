use std::fmt;
use std::ops::{BitAnd, BitOr, BitXor, Not};

pub const N: u32 = 4;

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct Function(pub u32);

impl Function {
    pub const TAUTOLOGY: Self = Function(2u32.pow(2u32.pow(N)) - 1);

    pub fn new(value: u32) -> Self {
        Function(value & Function::TAUTOLOGY.0)
    }
}

impl From<Function> for usize {
    fn from(f: Function) -> Self {
        f.0 as usize
    }
}

impl Not for Function {
    type Output = Self;

    fn not(self) -> Self::Output {
        Function((!self.0) & Function::TAUTOLOGY.0)
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
