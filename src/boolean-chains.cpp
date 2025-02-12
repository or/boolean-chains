#include "algorithm_l_extended.h"
#include "chain.h"
#include "expression.h"
#include "function.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

constexpr uint32_t N = 4;

template <uint32_t N>
std::vector<uint32_t> count_first_expressions_in_footprints(
    const Result<N> &algorithm_result,
    const std::vector<Function<N>> &target_functions) {

  std::vector<uint32_t> result(algorithm_result.first_expressions.size(), 0);

  for (const auto &f : target_functions) {
    for (size_t i = 0; i < algorithm_result.first_expressions.size(); i++) {
      if (algorithm_result.footprints[f.to_size_t()].get(i)) {
        result[i]++;
      }
    }
  }

  return result;
}

template <uint32_t N>
Expression<N>
pick_best_expression(std::mt19937_64 &rng,
                     const std::vector<Expression<N>> &first_expressions,
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

int main() {
  using namespace std::chrono;

  uint64_t seed =
      duration_cast<nanoseconds>(system_clock::now().time_since_epoch())
          .count();
  std::mt19937_64 rng(seed);

  Chain<N> chain({
      Function<N>(~0b1011011111100011),
      Function<N>(~0b1111100111100100),
      Function<N>(~0b1101111111110100),
      Function<N>(~0b1011011011011110),
      Function<N>(~0b1010001010111111),
      Function<N>(~0b1000111111110011),
      Function<N>(0b0011111011111111),
  });

  for (uint32_t k = 1; k <= N; k++) {
    uint32_t slice = (1u << (1u << (N - k))) + 1;
    Function<N> f = Function<N>(Function<N>::TAUTOLOGY / slice);
    chain.add(Expression<N>(f));
  }

  while (true) {
    chain.print();

    auto result2 = find_upper_bounds_and_footprints(chain);

    auto frequencies =
        count_first_expressions_in_footprints(result2, chain.targets);

    std::vector<size_t> range(result2.first_expressions.size());
    std::iota(range.begin(), range.end(), 0);

    std::sort(range.begin(), range.end(), [&frequencies](size_t a, size_t b) {
      return std::make_pair(-static_cast<int>(frequencies[a]),
                            -static_cast<int>(a)) <
             std::make_pair(-static_cast<int>(frequencies[b]),
                            -static_cast<int>(b));
    });

    // for (size_t i : range) {
    //     std::cout << result2.first_expressions[i].to_string() << " " <<
    //     frequencies[i] << std::endl;
    // }

    Expression<N> expr = pick_best_expression(rng, result2.first_expressions,
                                              range, frequencies);
    std::cout << "new expression selected: " << expr.to_string() << std::endl;

    chain.add(expr);

    uint32_t num_fulfilled_target_functions = 0;
    for (const auto &f : chain.targets) {
      if (chain.function_lookup.count(f)) {
        num_fulfilled_target_functions++;
      }
    }

    std::cout << "got " << chain.expressions.size() << " inputs, "
              << num_fulfilled_target_functions << " targets fulfilled"
              << std::endl;

    if (num_fulfilled_target_functions == chain.targets.size()) {
      break;
    }
  }

  chain.print();
  return 0;
}
