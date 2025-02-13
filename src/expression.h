#pragma once

#include "function.h"
#include <iostream>
#include <string>
#include <variant>

class Expression {
public:
  enum Type { Constant, And, Or, Xor, ButNot, NotBut };

private:
  Type type;
  Function f1, f2;

public:
  explicit Expression(Function f) : type(Type::Constant), f1(f), f2(0) {}
  Expression(Type op, Function lhs, Function rhs)
      : type(op), f1(lhs), f2(rhs) {}

  Function evaluate() const {
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
    return Function(0); // Should never reach here
  }

  const Type &get_type() const { return type; }

  const Function &get_operand1() const { return f1; }

  const Function &get_operand2() const { return f2; }

  std::string to_string() const {
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
template <> struct hash<Expression> {
  size_t operator()(const Expression &expr) const {
    size_t hash_value = 0;
    hash_value ^= std::hash<int>{}(static_cast<int>(expr.get_type()));
    hash_value ^= std::hash<Function>{}(expr.get_operand1()) << 1;
    hash_value ^= std::hash<Function>{}(expr.get_operand2()) << 2;
    return hash_value;
  }
};
} // namespace std
