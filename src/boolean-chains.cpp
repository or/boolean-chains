#include "algorithm_l_extended.h"
#include "chain.h"
#include "expression.h"
#include "function.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

std::vector<uint32_t>
count_first_expressions_in_footprints(const Result &algorithm_result,
                                      const Chain &chain) {

  std::vector<uint32_t> result(algorithm_result.first_expressions.size(), 0);

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

Expression
pick_best_expression(std::mt19937_64 &rng,
                     const std::vector<Expression> &first_expressions,
                     const std::vector<size_t> &range,
                     const std::vector<uint32_t> &frequencies) {

  return first_expressions[range[0]];

  // Example for selecting randomly with weighted probabilities
  // uint32_t total_frequency = std::accumulate(frequencies.begin(),
  // frequencies.end(), 0,
  //     [](uint32_t sum, uint32_t x) { return sum + x * x; });
  // std::uniform_int_distribution<uint32_t> dist(0, total_frequency - 1);
  // uint32_t value = dist(rng);
  // for (size_t index : range) {
  //     uint32_t weight = frequencies[index] * frequencies[index];
  //     if (value < weight) {
  //         return first_expressions[index];
  //     }
  //     value -= weight;
  // }
  // throw std::runtime_error("Unexpected case in weighted selection");
}

void find_optimal_chain(Chain &chain, size_t &current_best_length,
                        vector<uint32_t> &choices) {
  size_t num_fulfilled_target_functions = 0;
  for (const auto &f : chain.targets) {
    if (chain.function_lookup.count(f)) {
      num_fulfilled_target_functions++;
    }
  }

  if (num_fulfilled_target_functions == chain.targets.size()) {
    if (chain.expressions.size() < current_best_length) {
      std::cout << "New best chain found (" << chain.expressions.size()
                << "):\n"
                << std::flush;
      chain.print();
      current_best_length = chain.expressions.size();
    } else if (chain.expressions.size() <= 25) {
      chain.print();
    }
    // if (chain.expressions.size() == 25) {
    //   std::cout << "New chain found:\n";
    //   std::exit(1);
    // }
    return;
  }

  if (chain.expressions.size() + chain.targets.size() -
          num_fulfilled_target_functions >
      current_best_length) {
    return;
  }

  auto result = find_upper_bounds_and_footprints(chain);
  auto frequencies = count_first_expressions_in_footprints(result, chain);

  std::vector<size_t> range(result.first_expressions.size());
  std::iota(range.begin(), range.end(), 0);

  // std::sort(range.begin(), range.end(), [&frequencies](size_t a, size_t b) {
  //   return std::make_pair(-static_cast<int>(frequencies[a]),
  //                         -static_cast<int>(a)) <
  //          std::make_pair(-static_cast<int>(frequencies[b]),
  //                         -static_cast<int>(b));
  // });

  std::sort(range.begin(), range.end(), [&](size_t x, size_t y) {
    auto get_priority = [&](size_t index) {
      int min_value = 1000;
      for (const auto &y : chain.targets) {
        if (result.footprints[y.to_size_t()].get(index)) {
          min_value = std::min(
              min_value, static_cast<int>(result.upper_bounds[y.to_size_t()]));
        }
      }
      return std::make_tuple(min_value, -static_cast<int>(frequencies[index]),
                             -static_cast<int>(index));
    };
    return get_priority(x) < get_priority(y);
  });

  size_t current_length = chain.expressions.size();
  // bool added_new_expression = false;

  // for (const auto &expr : result.first_expressions) {
  //   auto f = expr.evaluate();
  //   if (chain.target_lookup.count(f) && !chain.function_lookup.count(f)) {
  //     chain.add(expr);
  //     added_new_expression = true;
  //   }
  // }

  // if (added_new_expression) {
  //   find_optimal_chain(chain, current_best_length);
  //   while (chain.expressions.size() > current_length) {
  //     chain.remove_last();
  //   }
  // }

  int max;
  if (chain.expressions.size() < 9) {
    max = 5;
  } else if (chain.expressions.size() < 14) {
    max = 3;
  } else {
    max = 1;
  }

  for (int i = 0; i < max; ++i) {
    choices.push_back(i);
    chain.add(result.first_expressions[range[i]]);
    if (chain.expressions.size() <= 14) {
      for (size_t j = 0; j < choices.size(); ++j) {
        std::cout << choices[j];
        if (j + 4 == 8) {
          std::cout << " |5| ";
        } else if (j + 4 == 13) {
          std::cout << " |3| ";
        } else if (j != choices.size() - 1) {
          std::cout << ", ";
        }
      }
      std::cout << " [best: " << current_best_length << "]" << std::endl
                << std::flush;
    }

    find_optimal_chain(chain, current_best_length, choices);
    choices.pop_back();
    chain.remove_last();
  }
}

int main() {
  using namespace std::chrono;

  uint64_t seed =
      duration_cast<nanoseconds>(system_clock::now().time_since_epoch())
          .count();
  std::mt19937_64 rng(seed);

  Chain chain({
      Function(~0b1011011111100011),
      Function(~0b1111100111100100),
      Function(~0b1101111111110100),
      Function(~0b1011011011011110),
      Function(~0b1010001010111111),
      Function(~0b1000111111110011),
      Function(0b0011111011111111),
  });

  for (uint32_t k = 1; k <= N; k++) {
    uint32_t slice = (1u << (1u << (N - k))) + 1;
    Function f = Function(Function::TAUTOLOGY / slice);
    chain.add(Expression(f));
  }

  // while (true) {
  //   chain.print();

  //   auto result2 = find_upper_bounds_and_footprints(chain);

  //   auto frequencies = count_first_expressions_in_footprints(result2, chain);

  //   std::vector<size_t> range(result2.first_expressions.size());
  //   std::iota(range.begin(), range.end(), 0);

  //   std::sort(range.begin(), range.end(), [&frequencies](size_t a, size_t b)
  //   {
  //     return std::make_pair(-static_cast<int>(frequencies[a]),
  //                           -static_cast<int>(a)) <
  //            std::make_pair(-static_cast<int>(frequencies[b]),
  //                           -static_cast<int>(b));
  //   });

  //   // for (size_t i : range) {
  //   //     std::cout << result2.first_expressions[i].to_string() << " " <<
  //   //     frequencies[i] << " (" << i << ")" << std::endl;
  //   // }

  //   Expression expr = pick_best_expression(rng, result2.first_expressions,
  //                                          range, frequencies);
  //   std::cout << "new expression selected: " << expr.to_string() <<
  //   std::endl;

  //   chain.add(expr);

  //   uint32_t num_fulfilled_target_functions = 0;
  //   for (const auto &f : chain.targets) {
  //     if (chain.function_lookup.count(f)) {
  //       num_fulfilled_target_functions++;
  //     }
  //   }

  //   std::cout << "got " << chain.expressions.size() << " inputs, "
  //             << num_fulfilled_target_functions << " targets fulfilled"
  //             << std::endl;

  //   if (num_fulfilled_target_functions == chain.targets.size()) {
  //     break;
  //   }
  // }

  // chain.print();
  size_t current_best_length = 1000;
  vector<uint32_t> choices;
  find_optimal_chain(chain, current_best_length, choices);
  return 0;
}
