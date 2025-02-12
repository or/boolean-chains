#pragma once

#include <vector>

using std::min;
using std::vector;

class BitSet {
private:
  vector<uint32_t> bit_set;

public:
  BitSet() : bit_set(vector<uint32_t>()) {}
  BitSet(const BitSet &other) : bit_set(other.bit_set) {}

  bool is_disjoint(const BitSet &other) const {
    for (size_t i = 0; i < min(bit_set.size(), other.bit_set.size()); ++i) {
      if (bit_set[i] & other.bit_set[i]) {
        return false;
      }
    }
    return true;
  }

  bool get(uint32_t bit) const {
    uint32_t index = bit >> 5;
    uint32_t bit_index = bit & 0b11111;
    if (bit_set.size() <= index) {
      return false;
    }
    return bit_set[index] & (1 << bit_index);
  }

  void insert(uint32_t bit) {
    uint32_t index = bit >> 5;
    uint32_t bit_index = bit & 0b11111;
    while (bit_set.size() <= index) {
      bit_set.push_back(0);
    }
    bit_set[index] |= (1 << bit_index);
  }

  void add(const BitSet &other) {
    while (bit_set.size() < other.bit_set.size()) {
      bit_set.push_back(0);
    }
    for (size_t i = 0; i < std::min(bit_set.size(), other.bit_set.size());
         ++i) {
      bit_set[i] |= other.bit_set[i];
    }
  }

  void intersect(const BitSet &other) {
    for (size_t i = 0; i < bit_set.size(); ++i) {
      if (i < other.bit_set.size()) {
        bit_set[i] &= other.bit_set[i];
      } else {
        bit_set[i] = 0;
      }
    }
  }
};
