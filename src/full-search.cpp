#include <bitset>
#include <cinttypes>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#ifndef CAPTURE_STATS
#define CAPTURE_STATS 1
#endif

#ifndef PLAN_MODE
#define PLAN_MODE 0
#endif

#if CAPTURE_STATS
#define CAPTURE_STATS_CALL(chain_size)                                         \
  {                                                                            \
    const auto new_expressions =                                               \
        expressions_size[chain_size] - expressions_size[chain_size - 1];       \
    stats_min_num_expressions[chain_size] +=                                   \
        (new_expressions < stats_min_num_expressions[chain_size]) *            \
        (new_expressions - stats_min_num_expressions[chain_size]);             \
    stats_max_num_expressions[chain_size] +=                                   \
        (new_expressions > stats_max_num_expressions[chain_size]) *            \
        (new_expressions - stats_max_num_expressions[chain_size]);             \
    stats_total_num_expressions[chain_size] += new_expressions;                \
    stats_num_data_points[chain_size]++;                                       \
  }
#else
#define CAPTURE_STATS_CALL(chain_size)
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

#define PRINT_PROGRESS(chain_size, last)                                       \
  for (size_t j = start_chain_length; j < chain_size; ++j) {                   \
    printf("%d, ", choices[j]);                                                \
  }                                                                            \
  printf("%d [best: %zu] %" PRIu64 "\n", last, current_best_length,            \
         total_chains);                                                        \
  fflush(stdout);

#define ADD_EXPRESSION(value, chain_size)                                      \
  expressions[expressions_size[chain_size]] = value;                           \
  expressions_size[chain_size] += seen[value];                                 \
  seen[value] = 0;

#define ADD_EXPRESSION_TARGET(value, chain_size)                               \
  {                                                                            \
    const uint32_t v = value;                                                  \
    const uint32_t a = seen[v] & target_lookup[v];                             \
    expressions[expressions_size[chain_size]] = v;                             \
    expressions_size[chain_size] += a;                                         \
    seen[v] &= ~a;                                                             \
  }

#define GENERATE_NEW_EXPRESSIONS(chain_size, add_expression)                   \
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
      add_expression(g0 & h, chain_size);                                      \
      add_expression(g1 & h, chain_size);                                      \
      add_expression(g2 & h, chain_size);                                      \
      add_expression(g3 & h, chain_size);                                      \
                                                                               \
      add_expression(not_g0 & h, chain_size);                                  \
      add_expression(not_g1 & h, chain_size);                                  \
      add_expression(not_g2 & h, chain_size);                                  \
      add_expression(not_g3 & h, chain_size);                                  \
                                                                               \
      add_expression(g0 & not_h, chain_size);                                  \
      add_expression(g1 & not_h, chain_size);                                  \
      add_expression(g2 & not_h, chain_size);                                  \
      add_expression(g3 & not_h, chain_size);                                  \
                                                                               \
      add_expression(g0 ^ h, chain_size);                                      \
      add_expression(g1 ^ h, chain_size);                                      \
      add_expression(g2 ^ h, chain_size);                                      \
      add_expression(g3 ^ h, chain_size);                                      \
                                                                               \
      add_expression(g0 | h, chain_size);                                      \
      add_expression(g1 | h, chain_size);                                      \
      add_expression(g2 | h, chain_size);                                      \
      add_expression(g3 | h, chain_size);                                      \
    }                                                                          \
                                                                               \
    for (; j < chain_size - 1; j++) {                                          \
      const uint32_t g = chain[j];                                             \
      const uint32_t not_g = ~chain[j];                                        \
      add_expression(g & h, chain_size);                                       \
      add_expression(not_g & h, chain_size);                                   \
      add_expression(g & not_h, chain_size);                                   \
      add_expression(g ^ h, chain_size);                                       \
      add_expression(g | h, chain_size);                                       \
    }                                                                          \
  }

void print_chain(const uint32_t *chain, const uint32_t *target_lookup,
                 const size_t chain_size) {
  printf("chain (%zu):\n", chain_size);
  for (size_t i = 0; i < chain_size; i++) {
    printf("x%zu", i + 1);
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

        printf(" = x%zu %c x%zu", j + 1, op, k + 1);
      }
    }
    printf(" = %s", std::bitset<N>(chain[i]).to_string().c_str());
    if (target_lookup[chain[i]]) {
      printf(" [target]");
    }
    printf("\n");
  }
}

void on_exit() {
  printf("total chains: %" PRIu64 "\n", total_chains);

#if CAPTURE_STATS
  printf("new expressions at chain length:\n");
  printf("                   n                       sum              avg     "
         "         "
         "min              max\n");

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

  chain[0] = 0b0000000011111111 >> (16 - N);
  chain[1] = 0b0000111100001111 >> (16 - N);
  chain[2] = 0b0011001100110011 >> (16 - N);
  chain[3] = 0b0101010101010101 >> (16 - N);
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
  printf("N = %d, MAX_LENGTH: %d, CAPTURE_STATS: %d\n", N, MAX_LENGTH,
         CAPTURE_STATS);
  printf("%d targets:\n", NUM_TARGETS);
  for (size_t i = 0; i < NUM_TARGETS; i++) {
    printf("  %s\n", std::bitset<N>(TARGETS[i]).to_string().c_str());
  }
  fflush(stdout);
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
  expressions_size[chain_size] = 0;
  for (size_t k = 1; k < chain_size; k++) {
    const uint32_t h = chain[k];
    const uint32_t not_h = ~h;
    for (size_t j = 0; j < k; j++) {
      const uint32_t g = chain[j];
      const uint32_t not_g = ~g;

      ADD_EXPRESSION(g & h, chain_size)
      ADD_EXPRESSION(g & not_h, chain_size)
      ADD_EXPRESSION(g ^ h, chain_size)
      ADD_EXPRESSION(g | h, chain_size)
      ADD_EXPRESSION(not_g & h, chain_size)
    }
  }

  // just to get the initial branch before the algorithm even starts
  CAPTURE_STATS_CALL(chain_size)
  chain_size++;

  stop_chain_size = start_chain_length;
  if (chunk_mode) {
    stop_chain_size = start_indices_size;
  }

  // restore progress
  if (start_indices_size > start_chain_length) {
    while (chain_size < start_indices_size - 1) {
      GENERATE_NEW_EXPRESSIONS(chain_size, ADD_EXPRESSION)
      choices[chain_size] = start_indices[chain_size];
      chain[chain_size] = expressions[choices[chain_size]];
      num_unfulfilled_targets -= target_lookup[chain[chain_size]];
      chain_size++;
    }

    GENERATE_NEW_EXPRESSIONS(chain_size, ADD_EXPRESSION)

    // will be incremented again in the main loop
    choices[chain_size] = start_indices[chain_size] - 1;

    // this will be counted again
    total_chains--;

    goto restore_progress;
  } else {
    // so that the first addition in the loop results in 0 for the first choice
    choices[chain_size] = 0xffffffff;
  }

  while (true) {
    if (__builtin_expect(chain_size + num_unfulfilled_targets == MAX_LENGTH,
                         1)) {
      size_t tmp_chain_size = chain_size;
      size_t generated_chain_size = chain_size - 1;
      uint32_t tmp_num_unfulfilled_targets = num_unfulfilled_targets;
      size_t j = choices[chain_size] + 1;

    next:
      while (__builtin_expect(tmp_chain_size < MAX_LENGTH, 1)) {
        GENERATE_NEW_EXPRESSIONS(tmp_chain_size, ADD_EXPRESSION_TARGET)
        generated_chain_size = tmp_chain_size;

        // CAPTURE_STATS_CALL(tmp_chain_size)

        for (; j < expressions_size[tmp_chain_size]; ++j) {
          total_chains++;
          if (__builtin_expect((total_chains & 0xffffffff) == 0, 0)) {
            PRINT_PROGRESS(tmp_chain_size, (uint32_t)j)
          }

          if (__builtin_expect(target_lookup[expressions[j]], 0)) {
            chain[tmp_chain_size] = expressions[j];
            tmp_num_unfulfilled_targets--;
            tmp_chain_size++;
            if (__builtin_expect(!tmp_num_unfulfilled_targets, 0)) {
              print_chain(chain, target_lookup, tmp_chain_size);
              goto leafs_done;
            }
            j++;
            goto next;
          }
        }
        break;
      }

    leafs_done:
      for (size_t i = expressions_size[chain_size - 1];
           i < expressions_size[generated_chain_size]; i++) {
        seen[expressions[i]] = 1;
      }

      chain_size--;
      num_unfulfilled_targets += target_lookup[chain[chain_size]];
      // if it was a target function, then we can skip all other choices at this
      // length, because the target function would now be in seen and prevent
      // any successful chain from here on; this massively reduces the search
      // space to about 50% the trick here is to simply add a large number to
      // the choices at that level if target_lookup is 1, this avoids branching
      choices[chain_size] += target_lookup[chain[chain_size]] << 16;
      if (__builtin_expect(chain_size < stop_chain_size, 0)) {
        return 0;
      }
    } else {
      GENERATE_NEW_EXPRESSIONS(chain_size, ADD_EXPRESSION)

      CAPTURE_STATS_CALL(chain_size)
    }

  restore_progress:
    while (true) {
      choices[chain_size]++;
      if (choices[chain_size] < expressions_size[chain_size]) {
        chain[chain_size] = expressions[choices[chain_size]];

#if PLAN_MODE
        if (chain_size + 1 - start_chain_length >= plan_depth) {
          printf("-c");
          for (size_t j = start_chain_length; j <= chain_size; ++j) {
            printf(" %d", choices[j]);
          }
          printf("\n");
          choices[chain_size]++;
          continue;
        }
#endif

        total_chains++;
        if (__builtin_expect((total_chains & 0xffffffff) == 0, 0)) {
          PRINT_PROGRESS(chain_size, choices[chain_size])
        }

        num_unfulfilled_targets -= target_lookup[chain[chain_size]];

        if (__builtin_expect(!num_unfulfilled_targets, 0)) {
          print_chain(chain, target_lookup, chain_size + 1);
          if (chain_size + 1 < current_best_length) {
            current_best_length = chain_size + 1;
          }
          // it must have been 1 to end up in this path, so we can just
          // increment num_unfulfilled_targets +=
          // target_lookup[chain[chain_size]];
          num_unfulfilled_targets++;
          goto done;
        }

        chain_size++;
        choices[chain_size] = choices[chain_size - 1];
        break;
      }

    done:
      for (size_t i = expressions_size[chain_size - 1];
           i < expressions_size[chain_size]; i++) {
        seen[expressions[i]] = 1;
      }

      chain_size--;
      num_unfulfilled_targets += target_lookup[chain[chain_size]];
      // if it was a target function, then we can skip all other choices at this
      // length, because the target function would now be in seen and prevent
      // any successful chain from here on; this massively reduces the search
      // space to about 50% the trick here is to simply add a large number to
      // the choices at that level if target_lookup is 1, this avoids branching
      choices[chain_size] += target_lookup[chain[chain_size]] << 16;
      if (__builtin_expect(chain_size < stop_chain_size, 0)) {
        return 0;
      }
    }
  }

  return 0;
}
