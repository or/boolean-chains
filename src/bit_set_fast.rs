#[derive(Clone, Debug, Default)]
pub struct BitSet {
    bit_set: Vec<u32>,
}

impl BitSet {
    pub fn new() -> Self {
        Self {
            bit_set: Vec::new(),
        }
    }

    pub fn is_disjoint(&self, other: &BitSet) -> bool {
        let min_len = self.bit_set.len().min(other.bit_set.len());
        for i in 0..min_len {
            if self.bit_set[i] & other.bit_set[i] != 0 {
                return false;
            }
        }
        true
    }

    pub fn get(&self, bit: u32) -> bool {
        let index = (bit >> 5) as usize;
        let bit_index = bit & 0b11111;
        if index >= self.bit_set.len() {
            return false;
        }
        self.bit_set[index] & (1 << bit_index) != 0
    }

    pub fn insert(&mut self, bit: u32) {
        let index = (bit >> 5) as usize;
        let bit_index = bit & 0b11111;
        if index >= self.bit_set.len() {
            self.bit_set.resize(index + 1, 0);
        }
        self.bit_set[index] |= 1 << bit_index;
    }

    pub fn add(&mut self, other: &BitSet) {
        if self.bit_set.len() < other.bit_set.len() {
            self.bit_set.resize(other.bit_set.len(), 0);
        }
        for i in 0..other.bit_set.len() {
            self.bit_set[i] |= other.bit_set[i];
        }
    }

    pub fn intersect(&mut self, other: &BitSet) {
        for i in 0..self.bit_set.len() {
            if i < other.bit_set.len() {
                self.bit_set[i] &= other.bit_set[i];
            } else {
                self.bit_set[i] = 0;
            }
        }
    }
}
