#include "bit_set_fast.h"
#include <algorithm>
#include <bitset>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <execinfo.h>
#include <signal.h>
#include <tuple>
#include <vector>

#ifndef CAPTURE_STATS
#define CAPTURE_STATS 1
#endif

#if CAPTURE_STATS
#define CAPTURE_STATS_CALL                                                     \
  {                                                                            \
    for (size_t i = 2; i < r; i++) {                                           \
      const auto new_expressions = levels_size[i];                             \
      if (new_expressions < stats_min_num_expressions[i]) {                    \
        stats_min_num_expressions[i] = new_expressions;                        \
      }                                                                        \
      if (new_expressions > stats_max_num_expressions[i]) {                    \
        stats_max_num_expressions[i] = new_expressions;                        \
      }                                                                        \
      stats_total_num_expressions[i] += new_expressions;                       \
      stats_num_data_points[i]++;                                              \
    }                                                                          \
  }
#else
#define CAPTURE_STATS_CALL
#endif

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
#if CAPTURE_STATS
#define UNDEFINED 0xffffffff
uint64_t stats_total_num_expressions[25] = {0};
uint32_t stats_min_num_expressions[25] = {UNDEFINED};
uint32_t stats_max_num_expressions[25] = {0};
uint64_t stats_num_data_points[25] = {0};
uint64_t stats_num_tries[25] = {0};
#endif

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
  printf("x%d", index + 1);
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

      printf(" = x%zu %c x%zu", j + 1, op, k + 1);
    }
  }
  printf(" = %s", std::bitset<N>(f).to_string().c_str());
  if (target_lookup[f]) {
    printf(" [target]");
  }
}

void print_chain(const uint32_t *chain, const size_t chain_size) {
  printf("hungry chain (%zu):\n", chain_size);
  for (size_t i = 0; i < chain_size; i++) {
    print_expression(chain, i, chain_size, chain[i]);
    printf("\n");
  }
  printf("\n");
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

void generate_first_expressions(const uint32_t *chain, const size_t chain_size,
                                uint32_t &c) {
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
}

void algorithm_l_with_footprints(const uint32_t *chain,
                                 const size_t chain_size) {
  memset(costs, 0xff, sizeof(costs));
  memset(footprints, 0, sizeof(footprints));
  uint32_t c = 1 << (N - 1);

  // for 0x00000000
  costs[0] = 0;
  c--;

  generate_first_expressions(chain, chain_size, c);

  // U3. Loop over r = 2, 3, ... while c > 0
  uint32_t r;
  for (r = 2; c > 0; ++r) {
    levels_size[r] = 0;

    bool all_targets_found = true;
    for (size_t i = 0; i < NUM_TARGETS; i++) {
      if (costs[TARGETS[i]] == 0xffffffff) {
        all_targets_found = false;
        break;
      }
    }

    if (all_targets_found) {
      break;
    }

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

#if CAPTURE_STATS
          stats_num_tries[r]++;
#endif

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

  CAPTURE_STATS_CALL
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

void on_exit() {
  printf("total chains: %" PRIu64 "\n", total_chains);

#if CAPTURE_STATS
  printf("new expressions at length in algorithm L:\n");
  printf("                   n                       sum              avg     "
         "         "
         "min              max        avg tries\n");

  for (int i = 2; i < 10; i++) {
    printf("%2d: %16" PRIu64 " %25" PRIu64 " %16" PRIu64 " %16" PRId32
           " %16" PRIu32 " %16" PRIu64 "\n",
           i, stats_num_data_points[i], stats_total_num_expressions[i],
           stats_num_data_points[i] == 0
               ? 0
               : stats_total_num_expressions[i] / stats_num_data_points[i],
           stats_min_num_expressions[i] == UNDEFINED
               ? 0
               : stats_min_num_expressions[i],
           stats_max_num_expressions[i],
           stats_num_data_points[i] == 0
               ? 0
               : stats_num_tries[i] / stats_num_data_points[i]);
  }
#endif
}

void signal_handler(int signal) { exit(signal); }

int main(int argc, char *argv[]) {
  bool chunk_mode = false;
  size_t stop_chain_size;
  uint32_t dummy_c = 0;
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
  bite_size[4] = 25;
  bite_size[5] = 25;
  bite_size[6] = 25;
  bite_size[7] = 25;
  bite_size[8] = 25;
  bite_size[9] = 10;
  bite_size[10] = 10;
  bite_size[11] = 9;
  bite_size[12] = 9;
  bite_size[13] = 8;
  bite_size[14] = 8;
  bite_size[15] = 7;
  bite_size[16] = 7;

  atexit(on_exit);
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

#if CAPTURE_STATS
  memset(stats_min_num_expressions, UNDEFINED,
         sizeof(stats_min_num_expressions));
#endif

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

  printf("hungry-search: N = %d, MAX_LENGTH: %d, CAPTURE_STATS: %d\n", N,
         MAX_LENGTH, CAPTURE_STATS);
  fflush(stdout);

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

  if (chain_size + num_unfulfilled_targets == MAX_LENGTH) {
    memset(costs, 0xff, sizeof(costs));
    generate_first_expressions(chain, chain_size, dummy_c);
    bool found = false;
    for (size_t i = 0; i < levels_size[1]; i++) {
      const uint32_t f = levels[1][i];
      if (target_lookup[f]) {
        expressions[chain_size][0] = f;
        priorities[chain_size].clear();
        priorities[chain_size].push_back(0);
        found = true;
        break;
      }
    }
    if (!found) {
      choices[chain_size] = 1 << 16;
    }
  } else {
    GENERATE_NEW_EXPRESSIONS
  }

restore_progress:
  do {
    choices[chain_size]++;
    while (choices[chain_size] < bite_size[chain_size] &&
           choices[chain_size] < priorities[chain_size].size()) {
      chain[chain_size] =
          expressions[chain_size][priorities[chain_size][choices[chain_size]]];

      total_chains++;
      if (__builtin_expect(chain_size <= 10, 0)) {
        for (size_t j = start_chain_length; j < chain_size; ++j) {
          printf("%d, ", choices[j]);
        }
        printf("%d [best: %zu] %" PRIu64 "\n", choices[chain_size],
               current_best_length, total_chains);
        fflush(stdout);
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
    // if it was a target function, then we can skip all other choices at this
    // length, because the target function needs to be taken at some point,
    // might as well be now
    //
    // the trick here is to simply add a large number to
    // the choices at that level if target_lookup is 1, this avoids branching
    choices[chain_size] += target_lookup[chain[chain_size]] << 16;
  } while (__builtin_expect(chain_size >= stop_chain_size, 1));

  return 0;
}
