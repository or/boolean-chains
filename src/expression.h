#pragma once

#include "function.h"
#include <iostream>
#include <string>
#include <variant>

using namespace std;

template <uint32_t N> class Expression {
public:
  enum Type { Constant, And, Or, Xor, ButNot, NotBut };

private:
  Type type;
  Function<N> f1, f2;

public:
  Expression() : type(Type::Constant), f1(0), f2(0) {}
  explicit Expression(Function<N> f) : type(Type::Constant), f1(f), f2(0) {}
  Expression(Type op, Function<N> lhs, Function<N> rhs)
      : type(op), f1(lhs), f2(rhs) {}

  Function<N> evaluate() const {
    switch (type) {
    case Type::Constant:
      return f1;
    case Type::And:
      return f1 & f2;
    case Type::Or:
      return f1 | f2;
    case Type::Xor:
      return f1 ^ f2;
    case Type::ButNot:
      return f1 & (~f2);
    case Type::NotBut:
      return (~f1) & f2;
    }
    return Function<N>(0); // Should never reach here
  }

  const Type &get_type() const { return type; }

  const Function<N> &get_operand1() const { return f1; }

  const Function<N> &get_operand2() const { return f2; }

  string to_string() const {
    switch (type) {
    case Type::Constant:
      return f1.to_string();
    case Type::And:
      return f1.to_string() + " & " + f2.to_string();
    case Type::Or:
      return f1.to_string() + " | " + f2.to_string();
    case Type::Xor:
      return f1.to_string() + " ^ " + f2.to_string();
    case Type::ButNot:
      return f1.to_string() + " > " + f2.to_string();
    case Type::NotBut:
      return f1.to_string() + " < " + f2.to_string();
    }
    return "";
  }

  bool operator==(const Expression &other) const {
    return type == other.type && f1 == other.f1 && f2 == other.f2;
  }
};

namespace std {
template <uint32_t N> struct hash<Expression<N>> {
  size_t operator()(const Expression<N> &expr) const {
    size_t hash_value = 0;
    hash_value ^= hash<int>{}(static_cast<int>(expr.get_type()));
    hash_value ^= hash<Function<N>>{}(expr.get_operand1()) << 1;
    hash_value ^= hash<Function<N>>{}(expr.get_operand2()) << 2;
    return hash_value;
  }
};
} // namespace std
