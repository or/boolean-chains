#include "algorithm_l_extended.h"
#include "chain.h"
#include "expression.h"
#include "function.h"
#include <cstdint>
#include <iostream>
#include <vector>
using namespace std;

const uint32_t N = 10;

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

void find_optimal_chain(Chain<N> &chain, size_t &current_best_length,
                        vector<uint32_t> &choices,
                        vector<uint32_t> &start_indices,
                        unordered_set<Expression<N>> &seen,
                        size_t num_fulfilled_target_functions) {
  if (num_fulfilled_target_functions == chain.targets.size()) {
    if (chain.expressions.size() < current_best_length) {
      cout << "New best chain found (" << chain.expressions.size() << "):\n"
           << flush;
      chain.print();
      current_best_length = chain.expressions.size();
    } else if (chain.expressions.size() < 16) {
      chain.print();
    }
    return;
  }

  if (chain.expressions.size() + chain.targets.size() -
          num_fulfilled_target_functions >
      15) {
    return;
  }

  if (chain.expressions.size() > 15) {
    return;
  }

  vector<Expression<N>> new_expressions;
  unordered_set<Function<N>> tmp_seen;

  for (size_t j = 0; j < chain.expressions.size(); j++) {
    for (size_t k = j + 1; k < chain.expressions.size(); k++) {
      Function<N> g = chain.expressions[j].function;
      Function<N> h = chain.expressions[k].function;

      for (const auto &expr : {
               Expression<N>(Expression<N>::Type::And, g, h),
               Expression<N>(Expression<N>::Type::NotBut, g, h),
               Expression<N>(Expression<N>::Type::ButNot, g, h),
               Expression<N>(Expression<N>::Type::Or, g, h),
               Expression<N>(Expression<N>::Type::Xor, g, h),
           }) {
        Function<N> f = expr.evaluate();

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

  unordered_set<Expression<N>> new_seen(seen);
  for (int i = 0; i < new_expressions.size(); ++i) {
    if (choices_vector_equals_start_indices(choices, start_indices) &&
        start_index_offset < start_indices.size() &&
        i < start_indices[start_index_offset]) {
      continue;
    }

    auto &new_expr = new_expressions[i];
    if (seen.find(new_expr) != seen.end()) {
      continue;
    }
    new_seen.insert(new_expr);

    choices.push_back(i);
    chain.add(new_expr);
    if (chain.expressions.size() <= 5) {
      for (size_t j = 0; j < choices.size(); ++j) {
        cout << choices[j];
        if (j != choices.size() - 1) {
          cout << ", ";
        }
      }
      cout << " [best: " << current_best_length << "]" << endl << flush;
    }

    size_t new_num_fulfilled = num_fulfilled_target_functions;
    if (chain.target_lookup.find(new_expr.evaluate()) !=
        chain.target_lookup.end()) {
      new_num_fulfilled++;
    }
    find_optimal_chain(chain, current_best_length, choices, start_indices,
                       new_seen, new_num_fulfilled);
    choices.pop_back();
    chain.remove_last();
  }
}

int main(int argc, char *argv[]) {
  Chain<N> chain({
      Function<N>(~0b1011011111),
      Function<N>(~0b1111100111),
      Function<N>(~0b1101111111),
      Function<N>(~0b1011011011),
      Function<N>(~0b1010001010),
      Function<N>(~0b1000111111),
      Function<N>(0b0011111011),
  });

  // parse a vector of integers passed as arguments
  vector<uint32_t> start_indices;
  for (int i = 1; i < argc; i++) {
    start_indices.push_back(atoi(argv[i]));
  }

  chain.add(Expression<N>(Function<N>(0b0000000011)));
  chain.add(Expression<N>(Function<N>(0b0000111100)));
  chain.add(Expression<N>(Function<N>(0b0011001100)));
  chain.add(Expression<N>(Function<N>(0b0101010101)));

  size_t current_best_length = 1000;
  vector<uint32_t> choices;
  unordered_set<Expression<N>> seen;
  find_optimal_chain(chain, current_best_length, choices, start_indices, seen,
                     0);
  return 0;
}
