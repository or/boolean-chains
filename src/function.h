#pragma once

#include <bitset>
#include <cstdint>
#include <string>

using namespace std;

template <uint32_t N> class Function {
public:
  uint32_t value;

  static const uint32_t TAUTOLOGY = (1 << N) - 1;

  explicit Function(uint32_t val) : value(val & TAUTOLOGY) {}

  uint32_t to_uint32_t() const { return value; }

  Function operator~() const { return Function(~value); }

  Function operator^(const Function &other) const {
    return Function(value ^ other.value);
  }

  Function operator&(const Function &other) const {
    return Function(value & other.value);
  }

  Function operator|(const Function &other) const {
    return Function(value | other.value);
  }

  size_t to_size_t() const { return static_cast<size_t>(value); }

  string to_string() const { return bitset<N>(value).to_string(); }

  bool operator==(const Function &other) const { return value == other.value; }
};

namespace std {
template <uint32_t N> struct hash<Function<N>> {
  size_t operator()(const Function<N> &f) const {
    return hash<uint32_t>{}(f.value);
  }
};
} // namespace std
