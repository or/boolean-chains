#pragma once

#include <algorithm>
#include <unordered_set>

using namespace std;

class BitSet {
private:
  unordered_set<uint32_t> bit_set;

public:
  BitSet() : bit_set(unordered_set<uint32_t>()) {}
  BitSet(const BitSet &other) : bit_set(other.bit_set) {}

  bool is_disjoint(const BitSet &other) const {
    for (uint32_t elem : bit_set) {
      if (other.bit_set.find(elem) != other.bit_set.end()) {
        return false;
      }
    }
    return true;
  }

  bool get(uint32_t bit) const { return bit_set.find(bit) != bit_set.end(); }

  void insert(uint32_t bit) { bit_set.insert(bit); }

  void add(const BitSet &other) {
    bit_set.insert(other.bit_set.begin(), other.bit_set.end());
  }

  void intersect(const BitSet &other) {
    erase_if(bit_set, [&other](uint32_t x) {
      return other.bit_set.find(x) == other.bit_set.end();
    });
  }
};
