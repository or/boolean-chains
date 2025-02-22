#include "algorithm_l_extended.h"
#include "chain.h"
#include "expression.h"
#include "function.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>
using namespace std;

bool choices_vector_equals_start_indices(vector<uint32_t> &choices,
                                         vector<uint32_t> &start_indices) {
  if (choices.size() > start_indices.size()) {
    return false;
  }
  for (int i = 0; i < choices.size(); i++) {
    if (choices[i] != start_indices[i]) {
      return false;
    }
  }
  return true;
}

void find_optimal_chain(Chain &chain, size_t &current_best_length,
                        vector<uint32_t> &choices,
                        vector<uint32_t> &start_indices,
                        unordered_set<Expression> &seen) {
  size_t num_fulfilled_target_functions = 0;
  for (const auto &f : chain.targets) {
    if (chain.function_lookup.count(f)) {
      num_fulfilled_target_functions++;
    }
  }

  if (num_fulfilled_target_functions == chain.targets.size()) {
    if (chain.expressions.size() < current_best_length) {
      cout << "New best chain found (" << chain.expressions.size() << "):\n"
           << flush;
      chain.print();
      current_best_length = chain.expressions.size();
    } else if (chain.expressions.size() < 18) {
      chain.print();
    }
    return;
  }

  if (chain.expressions.size() + chain.targets.size() -
          num_fulfilled_target_functions >
      18) {
    return;
  }

  if (chain.expressions.size() > 18) {
    return;
  }

  vector<Expression> new_expressions;
  unordered_set<Function> tmp_seen;

  for (size_t j = 0; j < chain.expressions.size(); j++) {
    for (size_t k = j + 1; k < chain.expressions.size(); k++) {
      Function g = chain.expressions[j].function;
      Function h = chain.expressions[k].function;

      for (const auto &expr : {
               Expression(Expression::Type::And, g, h),
               Expression(Expression::Type::NotBut, g, h),
               Expression(Expression::Type::ButNot, g, h),
               Expression(Expression::Type::Or, g, h),
               Expression(Expression::Type::Xor, g, h),
           }) {
        Function f = expr.evaluate();

        if (chain.function_lookup.find(f) != chain.function_lookup.end()) {
          continue;
        }

        if (tmp_seen.find(f) != tmp_seen.end()) {
          continue;
        }
        tmp_seen.insert(f);
        new_expressions.push_back(expr);
      }
    }
  }

  size_t current_length = chain.expressions.size();
  size_t start_index_offset = choices.size();

  unordered_set<Expression> new_seen(seen);
  for (int i = 0; i < new_expressions.size(); ++i) {
    if (choices_vector_equals_start_indices(choices, start_indices) &&
        start_index_offset < start_indices.size() &&
        i < start_indices[start_index_offset]) {
      continue;
    }

    // cout << "top " << max << " expressions:" << endl;
    // for (int j = 0; j < range.size() && j < 20; j++) {
    //   auto expr = result.first_expressions[range[j]];
    //   auto chain_expr = ChainExpression(chain.expressions.size(), expr);
    //   auto priority = get_priority(chain, result, frequencies, range[j]);
    //   cout << "  " << (i == j ? "* " : "  ")
    //        << chain.get_expression_as_str(chain_expr) << " ";
    //   cout << "[" << get<0>(priority) << " " << get<1>(priority) << " "
    //        << get<2>(priority) << "]" << endl;
    // }

    auto &new_expr = new_expressions[i];
    if (seen.find(new_expr) != seen.end()) {
      continue;
    }
    new_seen.insert(new_expr);

    choices.push_back(i);
    chain.add(new_expr);
    if (chain.expressions.size() <= 7) {
      for (size_t j = 0; j < choices.size(); ++j) {
        cout << choices[j];
        if (j != choices.size() - 1) {
          cout << ", ";
        }
      }
      cout << " [best: " << current_best_length << "]" << endl << flush;
    }

    find_optimal_chain(chain, current_best_length, choices, start_indices,
                       new_seen);
    choices.pop_back();
    chain.remove_last();
  }
}

int main(int argc, char *argv[]) {
  Chain chain({
      Function(~0b1011011111100011),
      Function(~0b1111100111100100),
      Function(~0b1101111111110100),
      Function(~0b1011011011011110),
      Function(~0b1010001010111111),
      Function(~0b1000111111110011),
      Function(0b0011111011111111),
  });

  // parse a vector of integers passed as arguments
  vector<uint32_t> start_indices;
  for (int i = 1; i < argc; i++) {
    start_indices.push_back(atoi(argv[i]));
  }

  chain.add(Expression(Function(0b0000000011111111)));
  chain.add(Expression(Function(0b0000111100001111)));
  chain.add(Expression(Function(0b0011001100110011)));
  chain.add(Expression(Function(0b0101010101010101)));

  size_t current_best_length = 1000;
  vector<uint32_t> choices;
  unordered_set<Expression> seen;
  find_optimal_chain(chain, current_best_length, choices, start_indices, seen);
  return 0;
}
