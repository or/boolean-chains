#pragma once

#include "expression.h"
#include "function.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

struct ChainExpression {
  size_t index;
  Function function;
  Expression expression;

  ChainExpression(size_t idx, const Expression &expr)
      : index(idx), function(expr.evaluate()), expression(expr) {}
};

class Chain {
public:
  vector<ChainExpression> expressions;
  unordered_map<Function, size_t> function_lookup;
  vector<Function> targets;
  unordered_set<Function> target_lookup;

  explicit Chain(const vector<Function> &target_functions)
      : targets(target_functions),
        target_lookup(target_functions.begin(), target_functions.end()) {}

  void add_target(const Function &f) {
    targets.push_back(f);
    target_lookup.insert(f);
  }

  void add(const Expression &expr) {
    size_t index = expressions.size();
    Function f = expr.evaluate();
    expressions.emplace_back(index, expr);
    function_lookup[f] = index;
  }

  void remove_last() {
    if (!expressions.empty()) {
      ChainExpression expr = expressions.back();
      expressions.pop_back();
      function_lookup.erase(expr.function);
    }
  }

  std::string get_name(const ChainExpression &chain_expr) const {
    return "x" + std::to_string(chain_expr.index + 1);
  }

  std::string get_name_for_function(const Function &f) const {
    auto it = function_lookup.find(f);
    return (it != function_lookup.end()) ? get_name(expressions[it->second])
                                         : "unknown";
  }

  std::string get_expression_as_str(const ChainExpression &chain_expr) const {
    std::string name = get_name(chain_expr);
    std::string is_target =
        target_lookup.count(chain_expr.function) ? " [target]" : "";

    switch (chain_expr.expression.get_type()) {
    case Expression::Type::Constant:
      return name + " = " + chain_expr.expression.get_operand1().to_string();
    case Expression::Type::And:
      return name + " = " +
             get_name_for_function(chain_expr.expression.get_operand1()) +
             " & " +
             get_name_for_function(chain_expr.expression.get_operand2()) +
             " = " + chain_expr.function.to_string() + is_target;
    case Expression::Type::Or:
      return name + " = " +
             get_name_for_function(chain_expr.expression.get_operand1()) +
             " | " +
             get_name_for_function(chain_expr.expression.get_operand2()) +
             " = " + chain_expr.function.to_string() + is_target;
    case Expression::Type::Xor:
      return name + " = " +
             get_name_for_function(chain_expr.expression.get_operand1()) +
             " ^ " +
             get_name_for_function(chain_expr.expression.get_operand2()) +
             " = " + chain_expr.function.to_string() + is_target;
    case Expression::Type::ButNot:
      return name + " = " +
             get_name_for_function(chain_expr.expression.get_operand1()) +
             " > " +
             get_name_for_function(chain_expr.expression.get_operand2()) +
             " = " + chain_expr.function.to_string() + is_target;
    case Expression::Type::NotBut:
      return name + " = " +
             get_name_for_function(chain_expr.expression.get_operand1()) +
             " < " +
             get_name_for_function(chain_expr.expression.get_operand2()) +
             " = " + chain_expr.function.to_string() + is_target;
    default:
      return name + " = unknown";
    }
  }

  void print() const {
    std::cout << "chain:\n";
    for (const auto &chain_expr : expressions) {
      std::cout << "  " << get_expression_as_str(chain_expr) << '\n';
    }
    std::cout << std::flush;
  }
};
