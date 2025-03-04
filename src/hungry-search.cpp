#include "bit_set_fast.h"
#include <algorithm>
#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <execinfo.h>
#include <iostream>
#include <signal.h>
#include <tuple>
#include <vector>
using namespace std;

constexpr uint32_t N = 16;
constexpr uint32_t SIZE = 1 << (N - 1);
constexpr uint32_t MAX_LENGTH = 22;
constexpr uint32_t TAUTOLOGY = (1 << N) - 1;
constexpr uint32_t TARGET_1 =
    ((~(uint32_t)0b1011011111100011) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGET_2 =
    ((~(uint32_t)0b1111100111100100) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGET_3 =
    ((~(uint32_t)0b1101111111110100) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGET_4 =
    ((~(uint32_t)0b1011011011011110) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGET_5 =
    ((~(uint32_t)0b1010001010111111) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGET_6 =
    ((~(uint32_t)0b1000111111110011) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGET_7 =
    (((uint32_t)0b0011111011111111) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGETS[] = {
    TARGET_1, TARGET_2, TARGET_3, TARGET_4, TARGET_5, TARGET_6, TARGET_7,
};
constexpr uint32_t NUM_TARGETS = sizeof(TARGETS) / sizeof(uint32_t);

uint32_t start_chain_length;
uint64_t total_chains = 0;

BitSet footprints[SIZE];
uint32_t costs[SIZE] __attribute__((aligned(64))) = {0};
uint32_t levels[50][50000];
size_t levels_size[10] = {0};
uint32_t frequencies[50000];
uint32_t target_lookup[SIZE] __attribute__((aligned(64))) = {0};
uint32_t chain_lookup[SIZE] __attribute__((aligned(64))) = {0};

#define ADD_FIRST_EXPRESSION(value)                                            \
  {                                                                            \
    uint32_t f = value;                                                        \
    if (costs[f] == 0xffffffff) {                                              \
      costs[f] = 1;                                                            \
      levels[1][levels_size[1]] = f;                                           \
      footprints[f].insert(levels_size[1]);                                    \
      levels_size[1]++;                                                        \
      c--;                                                                     \
    }                                                                          \
  }

void print_expression(const uint32_t *chain, const uint32_t index,
                      const size_t chain_size, const uint32_t f) {
  cout << "x" << index + 1;
  for (size_t j = 0; j < index; j++) {
    for (size_t k = j + 1; k < index; k++) {
      char op = 0;
      if (f == (chain[j] & chain[k])) {
        op = '&';
      } else if (f == (chain[j] | chain[k])) {
        op = '|';
      } else if (f == (chain[j] ^ chain[k])) {
        op = '^';
      } else if (f == ((~chain[j]) & chain[k])) {
        op = '<';
      } else if (f == (chain[j] & (~chain[k]))) {
        op = '>';
      } else {
        continue;
      }

      cout << " = " << "x" << j + 1 << " " << op << " x" << k + 1;
    }
  }
  cout << " = " << bitset<N>(f).to_string();
  if (target_lookup[f]) {
    cout << " [target]";
  }
}

void print_chain(const uint32_t *chain, const size_t chain_size) {
  cout << "hungry chain (" << chain_size << "):" << endl;
  for (size_t i = 0; i < chain_size; i++) {
    print_expression(chain, i, chain_size, chain[i]);
    cout << endl;
  }
  cout << endl;
}

void erase(uint32_t *array, size_t &array_size, uint32_t f) {
  for (size_t i = 0; i < array_size; i++) {
    if (array[i] == f) {
      if (i < array_size - 1) {
        array[i] = array[array_size - 1];
      }
      array_size--;
      return;
    }
  }
}

void algorithm_l_with_footprints(const uint32_t *chain,
                                 const size_t chain_size) {
  memset(costs, 0xff, sizeof(costs));
  memset(footprints, 0, sizeof(footprints));
  uint32_t c = 1 << (N - 1);

  // for 0x00000000
  costs[0] = 0;
  c--;

  // U1. Initialize
  levels_size[0] = 0;
  for (size_t i = 0; i < chain_size; i++) {
    costs[chain[i]] = 0;
    levels[0][levels_size[0]] = chain[i];
    levels_size[0]++;
    c--;
  }

  // U2. Iterate through all pairs of chain expressions
  levels_size[1] = 0;
  for (size_t j = 0; j < chain_size; j++) {
    uint32_t g = chain[j];
    uint32_t not_g = ~g;
    for (size_t k = j + 1; k < chain_size; k++) {
      uint32_t h = chain[k];
      uint32_t not_h = ~h;

      ADD_FIRST_EXPRESSION(g & h);
      ADD_FIRST_EXPRESSION(not_g & h);
      ADD_FIRST_EXPRESSION(g & not_h);
      ADD_FIRST_EXPRESSION(g | h);
      ADD_FIRST_EXPRESSION(g ^ h);
    }
  }

  // U3. Loop over r = 2, 3, ... while c > 0
  for (uint32_t r = 2; c > 0; ++r) {
    levels_size[r] = 0;

    // U4. Loop over j = [(r-1)/2], ..., 0, k = r - 1 - j
    for (int j = (r - 1) / 2; j >= 0; --j) {
      uint32_t k = r - 1 - j;

      for (size_t gi = 0; gi < levels_size[j]; ++gi) {
        uint32_t g = levels[j][gi];
        uint32_t not_g = ~g;
        size_t start_hi = (j == k) ? gi + 1 : 0;

        for (size_t hi = start_hi; hi < levels_size[k]; ++hi) {
          uint32_t h = levels[k][hi];
          uint32_t not_h = ~h;

          BitSet v(footprints[g]);
          uint32_t u;

          if (footprints[g].is_disjoint(footprints[h])) {
            u = r;
            v.add(footprints[h]);
          } else {
            u = r - 1;
            v.intersect(footprints[h]);
          }

          for (const uint32_t &f :
               {g & h, not_g & h, g & not_h, g | h, g ^ h}) {
            if (costs[f] == 0xffffffff) {
              costs[f] = u;
              levels[u][levels_size[u]] = f;
              levels_size[u]++;
              footprints[f] = v;
              c--;
            } else if (costs[f] > u) {
              const uint32_t previous_u = costs[f];
              erase(levels[previous_u], levels_size[previous_u], f);
              costs[f] = u;
              levels[u][levels_size[u]] = f;
              levels_size[u]++;
              footprints[f] = v;
            } else if (costs[f] == u) {
              footprints[f].add(v);
            }
          }
        }
      }
    }
  }
}

void count_first_expressions_in_footprints() {
  memset(frequencies, 0, sizeof(uint32_t) * levels_size[1]);

  for (const uint32_t &f : TARGETS) {
    if (chain_lookup[f]) {
      continue;
    }
    for (size_t i = 0; i < levels_size[1]; i++) {
      if (footprints[f].get(i)) {
        frequencies[i]++;
      }
    }
  }
}

auto get_priority(const size_t index) {
  int min_value = 1000;
  for (const auto &y : TARGETS) {
    if (footprints[y].get(index)) {
      min_value = min(min_value, static_cast<int>(costs[y]));
    }
  }
  return make_tuple(min_value, -static_cast<int>(frequencies[index]),
                    -static_cast<int>(index));
};

void on_exit() { cout << "total chains: " << total_chains << endl; }

void signal_handler(int signal) { exit(signal); }

int main(int argc, char *argv[]) {
  uint32_t chain[25] __attribute__((aligned(64)));
  uint32_t num_unfulfilled_targets = NUM_TARGETS;
  vector<uint32_t> algorithm_result;
  algorithm_result.reserve(1000);

  atexit(on_exit);
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  for (size_t i = 0; i < NUM_TARGETS; i++) {
    target_lookup[TARGETS[i]] = 1;
  }

  chain[0] = 0b0000000011111111 >> (16 - N);
  chain[1] = 0b0000111100001111 >> (16 - N);
  chain[2] = 0b0011001100110011 >> (16 - N);
  chain[3] = 0b0101010101010101 >> (16 - N);
  size_t chain_size = 4;
  start_chain_length = chain_size;

  for (size_t i = 0; i < chain_size; i++) {
    chain_lookup[chain[i]] = 1;
  }

  while (num_unfulfilled_targets) {
    cout << "chain length: " << chain_size
         << ", missing targets: " << num_unfulfilled_targets << endl;

    // do algorithm L
    algorithm_l_with_footprints(chain, chain_size);

    algorithm_result.clear();
    for (size_t i = 0; i < levels_size[1]; i++) {
      algorithm_result.push_back(i);
    }

    count_first_expressions_in_footprints();

    sort(algorithm_result.begin(), algorithm_result.end(),
         [&](size_t x, size_t y) { return get_priority(x) < get_priority(y); });

    cout << "top expressions (of " << algorithm_result.size() << "):" << endl;
    for (int j = 0; j < algorithm_result.size() && j < 5; j++) {
      auto f = levels[1][algorithm_result[j]];
      auto priority = get_priority(algorithm_result[j]);
      cout << "   ";
      print_expression(chain, chain_size, chain_size, f);
      cout << " [" << get<0>(priority) << " " << get<1>(priority) << " "
           << get<2>(priority) << "]" << endl;
    }

    // pick next choice

    chain[chain_size] = levels[1][algorithm_result[0]];
    chain_lookup[chain[chain_size]] = 1;
    if (target_lookup[chain[chain_size]]) {
      num_unfulfilled_targets--;
    }
    chain_size++;
  }

  print_chain(chain, chain_size);

  return 0;
}
