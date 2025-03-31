#include <bitset>
#include <cinttypes>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
using namespace std;

#ifndef CAPTURE_STATS
#define CAPTURE_STATS 1
#endif

#ifndef PLAN_MODE
#define PLAN_MODE 0
#endif

#if CAPTURE_STATS
#define CAPTURE_STATS_CALL                                                     \
  {                                                                            \
    const auto new_expressions =                                               \
        expressions_size[chain_size] - expressions_size[chain_size - 1];       \
    if (new_expressions < stats_min_num_expressions[chain_size]) {             \
      stats_min_num_expressions[chain_size] = new_expressions;                 \
    }                                                                          \
    if (new_expressions > stats_max_num_expressions[chain_size]) {             \
      stats_max_num_expressions[chain_size] = new_expressions;                 \
    }                                                                          \
    stats_total_num_expressions[chain_size] += new_expressions;                \
    stats_num_data_points[chain_size]++;                                       \
  }
#else
#define CAPTURE_STATS_CALL
#endif

constexpr uint32_t N = 16;
constexpr uint32_t SIZE = 1 << (N - 1);
constexpr uint32_t MAX_LENGTH = 12;
constexpr uint32_t TAUTOLOGY = (1 << 10) - 1;
constexpr uint32_t TARGET_1 =
    (((uint32_t)0b0001011001) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGET_2 =
    (((uint32_t)0b0000010001) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGET_3 =
    (((uint32_t)0b0111010110) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGET_4 =
    (((uint32_t)0b0011000010) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGET_5 =
    (((uint32_t)0b1010001010111111) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGET_6 =
    (((uint32_t)0b1000111111110011) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGET_7 =
    (((uint32_t)0b0011111011111111) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGETS[] = {TARGET_1, TARGET_2, TARGET_3, TARGET_4};
constexpr uint32_t NUM_TARGETS = sizeof(TARGETS) / sizeof(uint32_t);

#if PLAN_MODE
size_t plan_depth = 1;
#endif

uint32_t start_chain_length;
uint64_t total_chains = 0;
#if CAPTURE_STATS
#define UNDEFINED 0xffffffff
uint64_t stats_total_num_expressions[25] = {0};
uint32_t stats_min_num_expressions[25] = {UNDEFINED};
uint32_t stats_max_num_expressions[25] = {0};
uint64_t stats_num_data_points[25] = {0};
#endif

#define ADD_EXPRESSION(value)                                                  \
  expressions[expressions_size[chain_size]] = value;                           \
  expressions_size[chain_size] += seen[value];                                 \
  seen[value] = 0;

#define GENERATE_NEW_EXPRESSIONS                                               \
  {                                                                            \
    expressions_size[chain_size] = expressions_size[chain_size - 1];           \
    const uint32_t h = chain[chain_size - 1];                                  \
    const uint32_t not_h = ~h;                                                 \
                                                                               \
    size_t j = 0;                                                              \
    for (; j < chain_size - 4; j += 4) {                                       \
      const uint32_t g0 = chain[j], g1 = chain[j + 1], g2 = chain[j + 2],      \
                     g3 = chain[j + 3];                                        \
      const uint32_t not_g0 = ~g0, not_g1 = ~g1, not_g2 = ~g2, not_g3 = ~g3;   \
                                                                               \
      ADD_EXPRESSION(g0 & h);                                                  \
      ADD_EXPRESSION(g1 & h);                                                  \
      ADD_EXPRESSION(g2 & h);                                                  \
      ADD_EXPRESSION(g3 & h);                                                  \
                                                                               \
      ADD_EXPRESSION(not_g0 & h);                                              \
      ADD_EXPRESSION(not_g1 & h);                                              \
      ADD_EXPRESSION(not_g2 & h);                                              \
      ADD_EXPRESSION(not_g3 & h);                                              \
                                                                               \
      ADD_EXPRESSION(g0 & not_h);                                              \
      ADD_EXPRESSION(g1 & not_h);                                              \
      ADD_EXPRESSION(g2 & not_h);                                              \
      ADD_EXPRESSION(g3 & not_h);                                              \
                                                                               \
      ADD_EXPRESSION(g0 ^ h);                                                  \
      ADD_EXPRESSION(g1 ^ h);                                                  \
      ADD_EXPRESSION(g2 ^ h);                                                  \
      ADD_EXPRESSION(g3 ^ h);                                                  \
                                                                               \
      ADD_EXPRESSION(g0 | h);                                                  \
      ADD_EXPRESSION(g1 | h);                                                  \
      ADD_EXPRESSION(g2 | h);                                                  \
      ADD_EXPRESSION(g3 | h);                                                  \
    }                                                                          \
                                                                               \
    for (; j < chain_size - 1; j++) {                                          \
      const uint32_t g = chain[j];                                             \
      const uint32_t not_g = ~chain[j];                                        \
      ADD_EXPRESSION(g & h);                                                   \
      ADD_EXPRESSION(not_g & h);                                               \
      ADD_EXPRESSION(g & not_h);                                               \
      ADD_EXPRESSION(g ^ h);                                                   \
      ADD_EXPRESSION(g | h);                                                   \
    }                                                                          \
  }

void print_chain(const uint32_t *chain, const uint32_t *target_lookup,
                 const size_t chain_size) {
  cout << "chain (" << chain_size << "):" << endl;
  for (size_t i = 0; i < chain_size; i++) {
    cout << "x" << i + 1;
    for (size_t j = 0; j < i; j++) {
      for (size_t k = j + 1; k < i; k++) {
        char op = 0;
        if (chain[i] == (chain[j] & chain[k])) {
          op = '&';
        } else if (chain[i] == (chain[j] | chain[k])) {
          op = '|';
        } else if (chain[i] == (chain[j] ^ chain[k])) {
          op = '^';
        } else if (chain[i] == ((~chain[j]) & chain[k])) {
          op = '<';
        } else if (chain[i] == (chain[j] & (~chain[k]))) {
          op = '>';
        } else {
          continue;
        }

        cout << " = " << "x" << j + 1 << " " << op << " x" << k + 1;
      }
    }
    cout << " = " << bitset<10>(chain[i]).to_string();
    if (target_lookup[chain[i]]) {
      cout << " [target]";
    }
    cout << endl;
  }
  cout << endl;
}

void on_exit() {
  cout << "total chains: " << total_chains << endl;

#if CAPTURE_STATS
  cout << "new expressions at chain length:" << endl;
  cout << "                   n                       sum              avg     "
          "         "
          "min              max"
       << endl;

  stats_total_num_expressions[start_chain_length] +=
      stats_total_num_expressions[start_chain_length - 1];
  stats_min_num_expressions[start_chain_length] +=
      stats_min_num_expressions[start_chain_length - 1];
  stats_max_num_expressions[start_chain_length] +=
      stats_min_num_expressions[start_chain_length - 1];
  for (int i = start_chain_length; i < MAX_LENGTH; i++) {
    printf("%2d: %16" PRIu64 " %25" PRIu64 " %16" PRIu64 " %16" PRId32
           " %16" PRIu32 "\n",
           i, stats_num_data_points[i], stats_total_num_expressions[i],
           stats_num_data_points[i] == 0
               ? 0
               : stats_total_num_expressions[i] / stats_num_data_points[i],
           stats_min_num_expressions[i] == UNDEFINED
               ? 0
               : stats_min_num_expressions[i],
           stats_max_num_expressions[i]);
  }
  cout << flush;
#endif
}

void signal_handler(int signal) { exit(signal); }

int main(int argc, char *argv[]) {
  bool chunk_mode = false;
  uint32_t num_unfulfilled_targets = NUM_TARGETS;
  size_t stop_chain_size;
  size_t current_best_length = 1000;
  size_t start_indices_size __attribute__((aligned(64))) = 0;
  uint16_t start_indices[100] __attribute__((aligned(64))) = {0};
  uint32_t choices[30] __attribute__((aligned(64)));
  uint32_t target_lookup[SIZE] __attribute__((aligned(64))) = {0};
  uint32_t seen[SIZE] __attribute__((aligned(64)));
  uint32_t chain[25] __attribute__((aligned(64)));
  uint32_t expressions[600] __attribute__((aligned(64)));
  uint32_t expressions_size[25] __attribute__((aligned(64)));

#if !PLAN_MODE
  atexit(on_exit);
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
#endif

  chain[0] = 0b0010100101 >> (16 - N);
  chain[1] = 0b0000010011 >> (16 - N);
  chain[2] = 0b0110001111 >> (16 - N);
  chain[3] = 0b0001111111 >> (16 - N);
  size_t chain_size = 4;
  start_chain_length = chain_size;

#if PLAN_MODE
  if (argc > 1) {
    plan_depth = atoi(argv[1]);
  }
#else
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
#endif

  for (size_t i = 0; i < SIZE; i++) {
    // flip the logic: 1 means unseen, 0 seen, that'll avoid one operation when
    // setting this flag
    seen[i] = 1;
  }

  for (size_t i = 0; i < NUM_TARGETS; i++) {
    target_lookup[TARGETS[i]] = 1;
  }

#if PLAN_MODE != 1
  cout << "N = " << N << ", MAX_LENGTH: " << MAX_LENGTH
       << ", CAPTURE_STATS: " << CAPTURE_STATS << endl;
  cout << NUM_TARGETS << " targets:" << endl;
  for (size_t i = 0; i < NUM_TARGETS; i++) {
    cout << "  " << bitset<10>(TARGETS[i]).to_string() << endl;
  }
#endif

#if CAPTURE_STATS
  memset(stats_min_num_expressions, UNDEFINED,
         sizeof(stats_min_num_expressions));
#endif
  seen[0] = 0;
  for (size_t i = 0; i < chain_size; i++) {
    seen[chain[i]] = 0;
  }

  chain_size--;
  expressions_size[chain_size - 1] = 0;
  expressions_size[chain_size] = 0;
  for (size_t k = 1; k < chain_size; k++) {
    const uint32_t h = chain[k];
    const uint32_t not_h = ~h;
    for (size_t j = 0; j < k; j++) {
      const uint32_t g = chain[j];
      const uint32_t not_g = ~g;

      ADD_EXPRESSION(g & h)
      ADD_EXPRESSION(g & not_h)
      ADD_EXPRESSION(g ^ h)
      ADD_EXPRESSION(g | h)
      ADD_EXPRESSION(not_g & h)
    }
  }

  // just to get the initial branch before the algorithm even starts
  CAPTURE_STATS_CALL
  chain_size++;

  stop_chain_size = start_chain_length;
  if (chunk_mode) {
    stop_chain_size = start_indices_size;
  }

  // restore progress
  if (start_indices_size > start_chain_length) {
    while (chain_size < start_indices_size - 1) {
      GENERATE_NEW_EXPRESSIONS
      choices[chain_size] = start_indices[chain_size];
      chain[chain_size] = expressions[choices[chain_size]];
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

  CAPTURE_STATS_CALL

restore_progress:
  do {
    choices[chain_size]++;
    while (choices[chain_size] < expressions_size[chain_size]) {
      chain[chain_size] = expressions[choices[chain_size]];

#if PLAN_MODE
      if (chain_size + 1 - start_chain_length >= plan_depth) {
        cout << "-c";
        for (size_t j = start_chain_length; j <= chain_size; ++j) {
          cout << " " << choices[j];
        }
        cout << endl;
        choices[chain_size]++;
        continue;
      }
#endif

      total_chains++;
      if (__builtin_expect((total_chains & 0xffffffff) == 0, 0)) {
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
        print_chain(chain, target_lookup, chain_size + 1);
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
      choices[chain_size] = choices[chain_size - 1];
      goto start;
    }

    for (size_t i = expressions_size[chain_size - 1];
         i < expressions_size[chain_size]; i++) {
      seen[expressions[i]] = 1;
    }

    chain_size--;
    num_unfulfilled_targets += target_lookup[chain[chain_size]];
    // if it was a target function, then we can skip all other choices at this
    // length, because the target function would now be in seen and prevent any
    // successful chain from here on; this massively reduces the search space to
    // about 50%
    // the trick here is to simply add a large number to the choices at that
    // level if target_lookup is 1, this avoids branching
    choices[chain_size] += target_lookup[chain[chain_size]] << 16;
  } while (__builtin_expect(chain_size >= stop_chain_size, 1));

  return 0;
}
