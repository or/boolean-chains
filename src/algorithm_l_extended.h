#pragma once

#include "bit_set.h"
#include "chain.h"
#include "expression.h"
#include "function.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using std::vector;

const uint32_t INFINITY_U32 = std::numeric_limits<uint32_t>::max();

template <uint32_t N> struct Result {
  vector<uint32_t> upper_bounds;
  vector<Expression<N>> first_expressions;
  vector<BitSet> footprints;
  vector<vector<Function<N>>> lists;
  vector<uint32_t> stats;

  Result(size_t num_functions)
      : upper_bounds(num_functions, INFINITY_U32), footprints(num_functions),
        lists(10), stats(10, 0) {
    upper_bounds[0] = 0;
    stats[0] = 1;
  }
};

template <uint32_t N>
Result<N> find_upper_bounds_and_footprints(const Chain<N> &chain) {
  size_t num_functions = 1 << ((1 << N) - 1);
  size_t max_num_first_expressions =
      chain.expressions.size() * (chain.expressions.size() - 1) * 5 / 2;

  // U1. Initialize result
  Result<N> result(num_functions);

  for (const auto &chain_expression : chain.expressions) {
    result.upper_bounds[chain_expression.function.to_size_t()] = 0;
    result.lists[0].push_back(chain_expression.function);
    result.stats[0]++;
  }

  uint32_t num_removed_infinities = 0;

  // U2. Iterate through all pairs of chain expressions
  for (size_t j = 0; j < chain.expressions.size(); j++) {
    for (size_t k = j + 1; k < chain.expressions.size(); k++) {
      Function<N> g = chain.expressions[j].function;
      Function<N> h = chain.expressions[k].function;

      for (const auto &expr :
           {Expression<N>(Expression<N>::Type::And, g, h),
            Expression<N>(Expression<N>::Type::Or, g, h),
            Expression<N>(Expression<N>::Type::Xor, g, h),
            Expression<N>(Expression<N>::Type::ButNot, g, h),
            Expression<N>(Expression<N>::Type::NotBut, g, h)}) {
        Function<N> f = expr.evaluate();

        if (chain.function_lookup.find(f) != chain.function_lookup.end()) {
          continue;
        }
        if (result.upper_bounds[f.to_size_t()] == INFINITY_U32) {
          num_removed_infinities++;
          result.upper_bounds[f.to_size_t()] = 1;
          result.lists[1].push_back(f);
          result.stats[1]++;

          result.footprints[f.to_size_t()].insert(
              result.first_expressions.size());
          result.first_expressions.push_back(expr);
        }
      }
    }
  }

  std::cout << "num first expressions: " << result.first_expressions.size()
            << " (max: " << max_num_first_expressions << ")\n";

  uint32_t c =
      num_functions - num_removed_infinities - chain.expressions.size() - 1;

  // U3. Loop over r = 2, 3, ... while c > 0
  for (uint32_t r = 2; c > 0; ++r) {
    if (result.lists.size() <= r)
      result.lists.emplace_back();
    if (result.stats.size() <= r)
      result.stats.emplace_back();

    // U4. Loop over j = [(r-1)/2], ..., 0, k = r - 1 - j
    for (int j = (r - 1) / 2; j >= 0; --j) {
      uint32_t k = r - 1 - j;

      for (size_t gi = 0; gi < result.lists[j].size(); ++gi) {
        size_t start_hi = (j == k) ? gi + 1 : 0;
        for (size_t hi = start_hi; hi < result.lists[k].size(); ++hi) {
          Function<N> g = result.lists[j][gi];
          Function<N> h = result.lists[k][hi];

          BitSet v(result.footprints[g.to_size_t()]);
          v.add(result.footprints[h.to_size_t()]);

          uint32_t u = result.footprints[g.to_size_t()].is_disjoint(
                           result.footprints[h.to_size_t()])
                           ? r
                           : r - 1;

          for (const auto &expr :
               {Expression<N>(Expression<N>::Type::And, g, h),
                Expression<N>(Expression<N>::Type::Or, g, h),
                Expression<N>(Expression<N>::Type::Xor, g, h),
                Expression<N>(Expression<N>::Type::ButNot, g, h),
                Expression<N>(Expression<N>::Type::NotBut, g, h)}) {
            Function<N> f = expr.evaluate();

            if (result.upper_bounds[f.to_size_t()] == INFINITY_U32) {
              result.upper_bounds[f.to_size_t()] = u;
              result.footprints[f.to_size_t()] = v;
              result.lists[u].push_back(f);
              result.stats[u]++;
              c--;
            } else if (result.upper_bounds[f.to_size_t()] > u) {
              uint32_t prev_u = result.upper_bounds[f.to_size_t()];
              auto &prev_list = result.lists[prev_u];
              std::erase(prev_list, f);
              result.stats[prev_u]--;
              result.lists[u].push_back(f);
              result.stats[u]++;
              result.upper_bounds[f.to_size_t()] = u;
              result.footprints[f.to_size_t()] = v;
            } else if (result.upper_bounds[f.to_size_t()] == u) {
              result.footprints[f.to_size_t()].add(v);
            }
          }
        }
      }
    }
  }

  return result;
}
