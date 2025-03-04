#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

using namespace std;

const size_t ARRAY_SIZE = 24;

class BitSet {
private:
  uint32_t bit_set[ARRAY_SIZE];

public:
  BitSet() { memset(bit_set, 0, sizeof(bit_set)); }
  BitSet(const BitSet &other) {
    memcpy(bit_set, other.bit_set, sizeof(bit_set));
  }

  bool is_disjoint(const BitSet &other) const {
    for (size_t i = 0; i < ARRAY_SIZE; ++i) {
      if (bit_set[i] & other.bit_set[i]) {
        return false;
      }
    }
    return true;
  }

  bool get(uint32_t bit) const {
    uint32_t index = bit >> 5;
    uint32_t bit_index = bit & 0b11111;
    if (ARRAY_SIZE <= index) {
      return false;
    }
    return bit_set[index] & (1 << bit_index);
  }

  void insert(uint32_t bit) {
    uint32_t index = bit >> 5;
    uint32_t bit_index = bit & 0b11111;
    bit_set[index] |= (1 << bit_index);
  }

  void add(const BitSet &other) {
    for (size_t i = 0; i < ARRAY_SIZE; ++i) {
      bit_set[i] |= other.bit_set[i];
    }
  }

  void intersect(const BitSet &other) {
    for (size_t i = 0; i < ARRAY_SIZE; ++i) {
      bit_set[i] &= other.bit_set[i];
    }
  }

  void clear() { memset(bit_set, 0, sizeof(bit_set)); }
};
