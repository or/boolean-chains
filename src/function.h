#pragma once

#include <bitset>
#include <cstdint>
#include <string>

const uint32_t N = 4;

class Function {
public:
  uint32_t value;

  static const uint32_t TAUTOLOGY = (1 << (1 << N)) - 1;

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

  std::string to_string() const { return std::bitset<16>(value).to_string(); }

  bool operator==(const Function &other) const { return value == other.value; }
};

namespace std {
template <> struct hash<Function> {
  size_t operator()(const Function &f) const {
    return std::hash<uint32_t>{}(f.value);
  }
};
} // namespace std
