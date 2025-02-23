#pragma once

#include "expression.h"
#include "function.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

template <uint32_t N> struct ChainExpression {
  size_t index;
  Function<N> function;
  Expression<N> expression;

  ChainExpression(size_t idx, const Expression<N> &expr)
      : index(idx), function(expr.evaluate()), expression(expr) {}
};

template <uint32_t N> class Chain {
public:
  vector<ChainExpression<N>> expressions;
  unordered_map<Function<N>, size_t> function_lookup;
  vector<Function<N>> targets;
  unordered_set<Function<N>> target_lookup;

  explicit Chain(const vector<Function<N>> &target_functions)
      : targets(target_functions),
        target_lookup(target_functions.begin(), target_functions.end()) {}

  void add_target(const Function<N> &f) {
    targets.push_back(f);
    target_lookup.insert(f);
  }

  void add(const Expression<N> &expr) {
    size_t index = expressions.size();
    Function<N> f = expr.evaluate();
    expressions.emplace_back(index, expr);
    function_lookup[f] = index;
  }

  void remove_last() {
    if (!expressions.empty()) {
      ChainExpression<N> expr = expressions.back();
      expressions.pop_back();
      function_lookup.erase(expr.function);
    }
  }

  string get_name(const ChainExpression<N> &chain_expr) const {
    return "x" + to_string(chain_expr.index + 1);
  }

  string get_name_for_function(const Function<N> &f) const {
    auto it = function_lookup.find(f);
    return (it != function_lookup.end()) ? get_name(expressions[it->second])
                                         : "unknown";
  }

  string get_expression_as_str(const ChainExpression<N> &chain_expr) const {
    string name = get_name(chain_expr);
    string is_target =
        target_lookup.count(chain_expr.function) ? " [target]" : "";

    switch (chain_expr.expression.get_type()) {
    case Expression<N>::Type::Constant:
      return name + " = " + chain_expr.expression.get_operand1().to_string();
    case Expression<N>::Type::And:
      return name + " = " +
             get_name_for_function(chain_expr.expression.get_operand1()) +
             " & " +
             get_name_for_function(chain_expr.expression.get_operand2()) +
             " = " + chain_expr.function.to_string() + is_target;
    case Expression<N>::Type::Or:
      return name + " = " +
             get_name_for_function(chain_expr.expression.get_operand1()) +
             " | " +
             get_name_for_function(chain_expr.expression.get_operand2()) +
             " = " + chain_expr.function.to_string() + is_target;
    case Expression<N>::Type::Xor:
      return name + " = " +
             get_name_for_function(chain_expr.expression.get_operand1()) +
             " ^ " +
             get_name_for_function(chain_expr.expression.get_operand2()) +
             " = " + chain_expr.function.to_string() + is_target;
    case Expression<N>::Type::ButNot:
      return name + " = " +
             get_name_for_function(chain_expr.expression.get_operand1()) +
             " > " +
             get_name_for_function(chain_expr.expression.get_operand2()) +
             " = " + chain_expr.function.to_string() + is_target;
    case Expression<N>::Type::NotBut:
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
    cout << "chain (" << expressions.size() << "):\n";
    for (const auto &chain_expr : expressions) {
      cout << "  " << get_expression_as_str(chain_expr) << '\n';
    }
    cout << flush;
  }
};
