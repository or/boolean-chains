use std::collections::HashSet;

#[derive(Clone, Debug, Default)]
pub struct BitSet {
    bit_set: HashSet<u32>,
}

impl BitSet {
    pub fn new() -> Self {
        Self {
            bit_set: HashSet::new(),
        }
    }

    pub fn is_disjoint(&self, other: &BitSet) -> bool {
        self.bit_set.is_disjoint(&other.bit_set)
    }

    pub fn get(&self, bit: u32) -> bool {
        self.bit_set.contains(&bit)
    }

    pub fn insert(&mut self, bit: u32) {
        self.bit_set.insert(bit);
    }

    pub fn add(&mut self, other: &BitSet) {
        self.bit_set.extend(&other.bit_set);
    }

    pub fn intersect(&mut self, other: &BitSet) {
        self.bit_set.retain(|x| other.bit_set.contains(x));
    }
}
