use std::fmt;
use std::ops::{BitAnd, BitOr, BitXor, Not};

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct Function<const B: u32>(pub u32);

impl<const N: u32> Function<N> {
    pub const TAUTOLOGY: Self = Function(2u32.pow(2u32.pow(N)) - 1);

    pub fn new(value: u32) -> Self {
        Function::<N>(value & Function::<N>::TAUTOLOGY.0)
    }
}

impl<const N: u32> From<Function<N>> for usize {
    fn from(f: Function<N>) -> Self {
        f.0 as usize
    }
}

impl<const N: u32> Not for Function<N> {
    type Output = Self;

    fn not(self) -> Self::Output {
        Function((!self.0) & Function::<N>::TAUTOLOGY.0)
    }
}

impl<const N: u32> BitXor for Function<N> {
    type Output = Self;

    fn bitxor(self, other: Self) -> Self::Output {
        Function(self.0 ^ other.0)
    }
}

impl<const N: u32> BitAnd for Function<N> {
    type Output = Self;

    fn bitand(self, other: Self) -> Self::Output {
        Function(self.0 & other.0)
    }
}

impl<const N: u32> BitOr for Function<N> {
    type Output = Self;

    fn bitor(self, other: Self) -> Self::Output {
        Function(self.0 | other.0)
    }
}

impl<const N: u32> fmt::Display for Function<N> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:016b}", self.0)
    }
}

impl<const N: u32> fmt::Debug for Function<N> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self)
    }
}
