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
constexpr uint32_t MAX_LENGTH = 26;
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

#define GENERATE_NEW_EXPRESSIONS                                               \
  algorithm_l_with_footprints(chain, chain_size);                              \
                                                                               \
  expressions_size[chain_size] = levels_size[1];                               \
  memcpy(expressions[chain_size], levels[1],                                   \
         sizeof(uint32_t) * expressions_size[chain_size]);                     \
                                                                               \
  priorities[chain_size].clear();                                              \
  for (size_t i = 0; i < expressions_size[chain_size]; i++) {                  \
    priorities[chain_size].push_back(i);                                       \
  }                                                                            \
                                                                               \
  count_first_expressions_in_footprints(expressions_size[chain_size]);         \
                                                                               \
  sort(priorities[chain_size].begin(), priorities[chain_size].end(),           \
       [&](size_t x, size_t y) { return get_priority(x) < get_priority(y); });

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

void count_first_expressions_in_footprints(const uint32_t expressions_size) {
  memset(frequencies, 0, sizeof(uint32_t) * expressions_size);

  for (const uint32_t &f : TARGETS) {
    for (size_t i = 0; i < expressions_size; i++) {
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
  bool chunk_mode = false;
  size_t stop_chain_size;
  uint32_t chain[MAX_LENGTH] __attribute__((aligned(64)));
  uint32_t num_unfulfilled_targets = NUM_TARGETS;
  uint32_t expressions[MAX_LENGTH][50000] __attribute__((aligned(64)));
  uint32_t expressions_size[MAX_LENGTH] __attribute__((aligned(64)));
  vector<uint32_t> priorities[MAX_LENGTH] __attribute__((aligned(64)));
  uint32_t choices[MAX_LENGTH] __attribute__((aligned(64)));
  size_t start_indices_size __attribute__((aligned(64))) = 0;
  uint16_t start_indices[100] __attribute__((aligned(64))) = {0};
  size_t current_best_length = 1000;
  size_t bite_size[MAX_LENGTH] = {0};
  for (size_t i = 0; i < MAX_LENGTH; i++) {
    bite_size[i] = 1;
  }
  bite_size[4] = 5;
  bite_size[5] = 5;
  bite_size[6] = 5;
  bite_size[7] = 5;
  bite_size[8] = 5;
  bite_size[9] = 3;
  bite_size[10] = 3;
  bite_size[11] = 3;
  bite_size[12] = 3;

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

  size_t start_i = 1;
  // -c for chunk mode, only complete one slice of the depth given by the
  // progress vector
  if (argc > 1 && strcmp(argv[1], "-c") == 0) {
    start_i++;
    chunk_mode = true;
  }

  for (size_t i = 0; i < chain_size; i++) {
    start_indices[start_indices_size++] = 0;
  }

  // read the progress vector, e.g 5 2 9, commas will be ignored: 5, 2, 9
  for (size_t i = start_i; i < argc; i++) {
    start_indices[start_indices_size++] = atoi(argv[i]);
  }

  stop_chain_size = start_chain_length;
  if (chunk_mode) {
    stop_chain_size = start_indices_size;
  }

  // restore progress
  if (start_indices_size > start_chain_length) {
    while (chain_size < start_indices_size - 1) {
      GENERATE_NEW_EXPRESSIONS
      choices[chain_size] = start_indices[chain_size];
      chain[chain_size] =
          expressions[chain_size][priorities[chain_size][choices[chain_size]]];
      num_unfulfilled_targets -= target_lookup[chain[chain_size]];
      chain_size++;
    }

    GENERATE_NEW_EXPRESSIONS

    // will be incremented again in the main loop
    choices[chain_size] = start_indices[chain_size] - 1;

    // this will be counted again
    total_chains--;

    // this must not be inside the while loop, otherwise it destroys the
    // compiler's ability to optimize, making it only about half as fast
    goto restore_progress;
  } else {
    // so that the first addition in the loop results in 0 for the first choice
    choices[chain_size] = 0xffffffff;
  }

start:

  GENERATE_NEW_EXPRESSIONS

  // cout << "top expressions (of " << priorities[chain_size].size()
  //      << "):" << endl;
  // for (int j = 0; j < priorities[chain_size].size() && j < 5; j++) {
  //   auto f = expressions[chain_size][priorities[chain_size][j]];
  //   auto priority = get_priority(priorities[chain_size][j]);
  //   cout << "   ";
  //   print_expression(chain, chain_size, chain_size, f);
  //   cout << " [" << get<0>(priority) << " " << get<1>(priority) << " "
  //        << get<2>(priority) << "]" << endl;
  // }

restore_progress:
  do {
    choices[chain_size]++;
    while (choices[chain_size] < bite_size[chain_size]) {
      chain[chain_size] =
          expressions[chain_size][priorities[chain_size][choices[chain_size]]];

      total_chains++;
      if (__builtin_expect(chain_size <= 11, 0)) {
        for (size_t j = start_chain_length; j < chain_size; ++j) {
          cout << choices[j] << ", ";
        }
        cout << choices[chain_size] << " [best: " << current_best_length << "] "
             << total_chains << endl;
        // exit(0);
      }

      num_unfulfilled_targets -= target_lookup[chain[chain_size]];
      if (chain_size + num_unfulfilled_targets >= MAX_LENGTH) {
        // no need to do this, as it must have been 0 to end up in this path
        // num_unfulfilled_targets += target_lookup[chain[chain_size]];
        choices[chain_size]++;
        continue;
      }

      if (__builtin_expect(!num_unfulfilled_targets, 0)) {
        print_chain(chain, chain_size + 1);
        if (chain_size + 1 < current_best_length) {
          current_best_length = chain_size + 1;
        }
        // it must have been 1 to end up in this path, so we can just increment
        // num_unfulfilled_targets += target_lookup[chain[chain_size]];
        num_unfulfilled_targets++;
        choices[chain_size]++;
        continue;
      }

      chain_size++;
      // will be incremented right away again on the first loop, starting at 0
      choices[chain_size] = 0xffffffff;
      goto start;
    }

    chain_size--;
    num_unfulfilled_targets += target_lookup[chain[chain_size]];
  } while (__builtin_expect(chain_size >= stop_chain_size, 1));

  return 0;
}
