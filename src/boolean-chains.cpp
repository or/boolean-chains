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
                        vector<uint32_t> &start_indices) {
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

  if (chain.expressions.size() > 26) {
    return;
  }

  auto result = find_upper_bounds_and_footprints(chain);
  auto frequencies = count_first_expressions_in_footprints(result, chain);

  vector<size_t> range(result.first_expressions.size());
  iota(range.begin(), range.end(), 0);

  sort(range.begin(), range.end(), [&](size_t x, size_t y) {
    auto get_priority = [&](size_t index) {
      int min_value = 1000;
      for (const auto &y : chain.targets) {
        if (result.footprints[y.to_size_t()].get(index)) {
          min_value = min(min_value,
                          static_cast<int>(result.upper_bounds[y.to_size_t()]));
        }
      }
      return make_tuple(min_value, -static_cast<int>(frequencies[index]),
                        -static_cast<int>(index));
    };
    return get_priority(x) < get_priority(y);
  });

  size_t current_length = chain.expressions.size();
  size_t start_index_offset = choices.size();

  int max;
  if (chain.expressions.size() < 9) {
    max = 5;
  } else if (chain.expressions.size() < 14) {
    max = 3;
  } else {
    max = 1;
  }

  if (max > range.size()) {
    max = range.size();
  }

  for (int i = 0; i < max; ++i) {
    if (choices_vector_equals_start_indices(choices, start_indices) &&
        start_index_offset < start_indices.size() &&
        i < start_indices[start_index_offset]) {
      continue;
    }

    choices.push_back(i);
    chain.add(result.first_expressions[range[i]]);
    if (chain.expressions.size() <= 14) {
      for (size_t j = 0; j < choices.size(); ++j) {
        cout << choices[j];
        if (j + 4 == 8) {
          cout << " |5| ";
        } else if (j + 4 == 13) {
          cout << " |3| ";
        } else if (j != choices.size() - 1) {
          cout << ", ";
        }
      }
      cout << " [best: " << current_best_length << "]" << endl << flush;
    }

    find_optimal_chain(chain, current_best_length, choices, start_indices);
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
  find_optimal_chain(chain, current_best_length, choices, start_indices);
  return 0;
}
