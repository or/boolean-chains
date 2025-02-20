#include "algorithm_l_extended.h"
#include "chain.h"
#include "expression.h"
#include "function.h"
#include <cstdint>
#include <iostream>
#include <numeric>
#include <unordered_set>
#include <vector>
using namespace std;

vector<uint32_t>
count_first_expressions_in_footprints(const Result &algorithm_result,
                                      const Chain &chain) {

  vector<uint32_t> result(algorithm_result.first_expressions.size(), 0);

  for (const auto &f : chain.targets) {
    if (chain.function_lookup.find(f) != chain.function_lookup.end()) {
      continue;
    }
    for (size_t i = 0; i < algorithm_result.first_expressions.size(); i++) {
      if (algorithm_result.footprints[f.to_size_t()].get(i)) {
        result[i]++;
      }
    }
  }

  return result;
}

auto get_priority(Chain &chain, Result &result, vector<uint32_t> &frequencies,
                  size_t index) {
  int min_value = 1000;
  for (const auto &y : chain.targets) {
    if (result.footprints[y.to_size_t()].get(index)) {
      min_value =
          min(min_value, static_cast<int>(result.upper_bounds[y.to_size_t()]));
    }
  }
  return make_tuple(min_value, -static_cast<int>(frequencies[index]),
                    -static_cast<int>(index));
};

void find_optimal_chain_rest(Chain &chain, size_t &current_best_length) {
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
    } else if (chain.expressions.size() <= 25) {
      chain.print();
    }
    return;
  }

  if (chain.expressions.size() + chain.targets.size() -
          num_fulfilled_target_functions >
      current_best_length) {
    return;
  }

  cout << chain.expressions.size() << endl;
  if (chain.expressions.size() > 24) {
    return;
  }

  auto result = find_upper_bounds_and_footprints(chain);
  auto frequencies = count_first_expressions_in_footprints(result, chain);

  vector<size_t> range(result.first_expressions.size());
  iota(range.begin(), range.end(), 0);

  sort(range.begin(), range.end(), [&](size_t x, size_t y) {
    return get_priority(chain, result, frequencies, x) <
           get_priority(chain, result, frequencies, y);
  });

  size_t current_length = chain.expressions.size();

  int max;
  if (chain.expressions.size() < 15) {
    max = 2;
  } else {
    max = 1;
  }

  if (max > range.size()) {
    max = range.size();
  }

  max = 1;

  for (int i = 0; i < max; ++i) {
    chain.add(result.first_expressions[range[i]]);
    find_optimal_chain_rest(chain, current_best_length);
    chain.remove_last();
  }
}

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

void find_optimal_chain(Chain &chain, vector<uint32_t> &choices,
                        vector<uint32_t> &start_indices,
                        unordered_set<string> &used_expressions,
                        uint32_t &num_paths, size_t &current_best) {
  int position = chain.expressions.size() - 4;
  if (position > 5) {
    find_optimal_chain_rest(chain, current_best);
    ++num_paths;
    return;
  }
  vector<Expression> next_expressions;
  unordered_set<Function> seen_functions;

  for (const auto &chain_expression : chain.expressions) {
    seen_functions.insert(chain_expression.function);
  }

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

        if (seen_functions.find(f) != seen_functions.end()) {
          continue;
        }
        seen_functions.insert(f);
        next_expressions.push_back(expr);
      }
    }
  }

  // cout << "position: " << position
  //      << " num next expressions: " << next_expressions.size() << endl;
  auto new_used_expressions = unordered_set<string>(used_expressions);
  size_t start_index_offset = choices.size();
  for (int i = 0; i < next_expressions.size(); ++i) {
    auto &next_expr = next_expressions[i];
    auto key = next_expr.to_string() + next_expr.evaluate().to_string();
    if (new_used_expressions.count(key)) {
      continue;
    }
    new_used_expressions.insert(key);

    if (choices_vector_equals_start_indices(choices, start_indices) &&
        start_index_offset < start_indices.size() &&
        i < start_indices[start_index_offset]) {
      continue;
    }

    choices.push_back(i);
    chain.add(next_expr);

    if (chain.expressions.size() <= 9) {
      for (size_t j = 0; j < choices.size(); ++j) {
        cout << choices[j];
        if (j != choices.size() - 1) {
          cout << ", ";
        }
      }
      cout << " " << "[paths: " << num_paths << "]" << endl;
    }

    find_optimal_chain(chain, choices, start_indices, new_used_expressions,
                       num_paths, current_best);
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
  unordered_set<string> used_expressions;
  uint32_t num_paths = 0;
  size_t current_best = 1000;
  find_optimal_chain(chain, choices, start_indices, used_expressions, num_paths,
                     current_best);

  return 0;
}
