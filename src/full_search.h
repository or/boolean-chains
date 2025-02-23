
#include "bit_set_template.h"
#include "chain.h"
#include "expression.h"
#include "function.h"
#include <cstdint>
#include <ctime>
#include <iostream>
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

template <uint32_t N, uint32_t S>
void find_optimal_chain(Chain<N> &chain, size_t &current_best_length,
                        vector<uint32_t> &choices,
                        vector<uint32_t> &start_indices, BitSet<S> &seen,
                        size_t num_fulfilled_target_functions,
                        uint32_t &total_chains, time_t &last_print,
                        size_t max_length) {
  total_chains++;
  if (num_fulfilled_target_functions == chain.targets.size()) {
    if (chain.expressions.size() < current_best_length) {
      cout << "New best chain found (" << chain.expressions.size() << "):\n"
           << flush;
      chain.print();
      current_best_length = chain.expressions.size();
    } else if (chain.expressions.size() == current_best_length) {
      chain.print();
    }
    return;
  }

  if (chain.expressions.size() + chain.targets.size() -
          num_fulfilled_target_functions >
      max_length) {
    return;
  }

  Expression<N> new_expressions[1000];
  size_t new_expressions_size = 0;
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
        if (seen.get(f.to_uint32_t())) {
          continue;
        }

        if (chain.function_lookup.find(f) != chain.function_lookup.end()) {
          continue;
        }

        if (tmp_seen.find(f) != tmp_seen.end()) {
          continue;
        }
        tmp_seen.insert(f);
        new_expressions[new_expressions_size] = expr;
        new_expressions_size++;
      }
    }
  }

  size_t current_length = chain.expressions.size();
  size_t start_index_offset = choices.size();
  uint32_t clean_up[1000];
  size_t clean_up_size = 0;
  for (int i = 0; i < new_expressions_size; ++i) {
    auto &new_expr = new_expressions[i];
    uint32_t ft = new_expr.evaluate().to_uint32_t();
    seen.insert(ft);
    clean_up[clean_up_size] = ft;
    clean_up_size++;

    if (choices_vector_equals_start_indices(choices, start_indices) &&
        start_index_offset < start_indices.size() &&
        i < start_indices[start_index_offset]) {
      continue;
    }

    choices.push_back(i);
    chain.add(new_expr);
    if (time(NULL) >= last_print + 10) {
      for (size_t j = 0; j < choices.size(); ++j) {
        cout << choices[j];
        if (j != choices.size() - 1) {
          cout << ", ";
        }
      }
      cout << " [best: " << current_best_length << "] " << total_chains << endl;
      last_print = time(NULL);
      // exit(1);
    }

    size_t new_num_fulfilled = num_fulfilled_target_functions;
    if (chain.target_lookup.find(new_expr.evaluate()) !=
        chain.target_lookup.end()) {
      new_num_fulfilled++;
    }
    find_optimal_chain(chain, current_best_length, choices, start_indices, seen,
                       new_num_fulfilled, total_chains, last_print, max_length);
    choices.pop_back();
    chain.remove_last();
  }

  for (int i = 0; i < clean_up_size; ++i) {
    seen.remove(clean_up[i]);
  }
}
