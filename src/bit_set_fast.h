#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

using namespace std;

// 12 uint64 = 768 bits total. ARRAY_SIZE chosen tightly for MAX_LENGTH=22;
// going smaller corrupts state. The storage is paired as 6 x (2 x uint64)
// vectors so clang emits NEON / SSE2 instead of scalar code for the bulk
// ops. Bit indexing in get() / insert() still treats bit_set as a flat
// array of uint64 lanes — vector lane order matches scalar array order
// on both ARM and x86.
const size_t ARRAY_SIZE = 12;
const size_t VEC_LANES = 2;
const size_t VEC_COUNT = ARRAY_SIZE / VEC_LANES;

typedef uint64_t bs_u64x2 __attribute__((vector_size(16)));

class BitSet {
private:
  union {
    bs_u64x2 vec[VEC_COUNT];
    uint64_t bit_set[ARRAY_SIZE];
  };
  // summary hash: bit (i % 64) is OR'd in for every set atom-index i.
  // (summary_A & summary_B) == 0 implies set(A) and set(B) are
  // disjoint, so it's a sound conservative pre-filter for is_disjoint.
  // After intersect we keep summary as the bitwise AND of the two
  // summaries, which is an over-approximation of summary(A ∩ B) and
  // also safe for the same reason (may report false overlap but never
  // false disjoint).
  uint64_t summary;

public:
  BitSet() {
    memset(bit_set, 0, sizeof(bit_set));
    summary = 0;
  }
  BitSet(const BitSet &other) {
    for (size_t i = 0; i < VEC_COUNT; ++i) {
      vec[i] = other.vec[i];
    }
    summary = other.summary;
  }

  bool is_disjoint(const BitSet &other) const {
    if ((summary & other.summary) == 0) {
      return true;
    }
    bs_u64x2 acc = {0, 0};
    for (size_t i = 0; i < VEC_COUNT; ++i) {
      acc |= vec[i] & other.vec[i];
    }
    return (acc[0] | acc[1]) == 0;
  }

  bool get(uint32_t bit) const {
    uint32_t index = bit >> 6;
    uint32_t bit_index = bit & 0b111111;
    if (ARRAY_SIZE <= index) {
      return false;
    }
    return bit_set[index] & (1ULL << bit_index);
  }

  void insert(uint32_t bit) {
    uint32_t index = bit >> 6;
    uint32_t bit_index = bit & 0b111111;
    bit_set[index] |= (1ULL << bit_index);
    summary |= (1ULL << (bit & 63));
  }

  void add(const BitSet &other) {
    for (size_t i = 0; i < VEC_COUNT; ++i) {
      vec[i] |= other.vec[i];
    }
    summary |= other.summary;
  }

  void intersect(const BitSet &other) {
    for (size_t i = 0; i < VEC_COUNT; ++i) {
      vec[i] &= other.vec[i];
    }
    summary &= other.summary;
  }

  void clear() {
    memset(bit_set, 0, sizeof(bit_set));
    summary = 0;
  }
};
