#include <bitset>
#include <cinttypes>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef __APPLE__
#include <sys/sysctl.h>
#endif
#if defined(__x86_64__)
#include <cpuid.h>
#endif

#ifndef CAPTURE_STATS
#define CAPTURE_STATS 1
#endif
#ifndef PLAN_MODE
#define PLAN_MODE 0
#endif
#define CHUNK_START_LENGTH 9

constexpr uint32_t N = 16;
constexpr uint32_t SIZE = 1 << (N - 1);
constexpr uint32_t MAX_LENGTH = 22;
constexpr uint32_t PRINT_PROGRESS_LENGTH = 10;
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

uint32_t plan_depth = 1;
uint32_t start_chain_length = 4;
uint64_t total_chains = 0;

#if CAPTURE_STATS
#define UNDEFINED 0xffffffff
uint64_t stats_total_num_expressions[25] = {0};
uint32_t stats_min_num_expressions[25] = {0};
uint32_t stats_max_num_expressions[25] = {0};
uint64_t stats_num_data_points[25] = {0};
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

#define PRINT_PROGRESS(chain_size, last)                                       \
  for (uint32_t _j = start_chain_length; _j < chain_size; ++_j) {              \
    printf("%d, ", choices[_j]);                                               \
  }                                                                            \
  printf("%d %" PRIu64 "\n", last, total_chains);                              \
  fflush(stdout);

// ---- Unseen accessors: plain ----

#define UNSEEN_PLAIN_GET(v) unseen_plain[(v)]
#define UNSEEN_PLAIN_CLEAR(v) unseen_plain[(v)] = 0
#define UNSEEN_PLAIN_CLEAR_COND(v, c) unseen_plain[(v)] &= ~(uint8_t)(c)
#define UNSEEN_PLAIN_RESTORE(v) unseen_plain[(v)] = 1
#define UNSEEN_PLAIN_INIT() memset(unseen_plain, 1, sizeof(unseen_plain))

// ---- Unseen accessors: bitset ----

#define UNSEEN_BITS_GET(v) ((unseen_bits[(v) >> 6] >> ((v) & 63)) & 1)
#define UNSEEN_BITS_CLEAR(v) unseen_bits[(v) >> 6] &= ~(1ULL << ((v) & 63))
#define UNSEEN_BITS_CLEAR_COND(v, c)                                           \
  unseen_bits[(v) >> 6] &= ~((uint64_t)(c) << ((v) & 63))
#define UNSEEN_BITS_RESTORE(v) unseen_bits[(v) >> 6] |= (1ULL << ((v) & 63))
#define UNSEEN_BITS_INIT() memset(unseen_bits, 0xFF, sizeof(unseen_bits))

// c: combined plain
#define UNSEEN_COMBINED_GET(v) (combined[(v)] & 1)
#define UNSEEN_COMBINED_CLEAR(v) combined[(v)] &= 0xFE
#define UNSEEN_COMBINED_CLEAR_COND(v, c) combined[(v)] &= ~(uint8_t)(c)
#define UNSEEN_COMBINED_RESTORE(v) combined[(v)] |= 1
#define UNSEEN_COMBINED_INIT() memset(combined, 1, sizeof(combined))

#define TARGET_COMBINED_GET(v) ((combined[(v)] >> 1) & 1)
#define TARGET_COMBINED_INIT()                                                 \
  do {                                                                         \
    for (uint32_t _i = 0; _i < NUM_TARGETS; _i++)                              \
      combined[TARGETS[_i]] |= 2;                                              \
  } while (0)

// ---- Target accessors: plain ----

#define TARGET_PLAIN_GET(v) target_plain[(v)]
#define TARGET_PLAIN_INIT()                                                    \
  do {                                                                         \
    memset(target_plain, 0, sizeof(target_plain));                             \
    for (uint32_t _i = 0; _i < NUM_TARGETS; _i++)                              \
      target_plain[TARGETS[_i]] = 1;                                           \
  } while (0)

// ---- Target accessors: bitset ----

#define TARGET_BITS_GET(v)                                                     \
  ((uint8_t)((target_bits[(v) >> 6] >> ((v) & 63)) & 1))
#define TARGET_BITS_INIT()                                                     \
  do {                                                                         \
    memset(target_bits, 0, sizeof(target_bits));                               \
    for (uint32_t _i = 0; _i < NUM_TARGETS; _i++)                              \
      target_bits[TARGETS[_i] >> 6] |= 1ULL << (TARGETS[_i] & 63);             \
  } while (0)

static void get_cpu_brand(char *brand, size_t max_len) {
#if defined(__APPLE__)
  size_t len = max_len;
  if (sysctlbyname("machdep.cpu.brand_string", brand, &len, nullptr, 0) == 0)
    return;
#endif
#if defined(__linux__)
  FILE *f = fopen("/proc/cpuinfo", "r");
  if (f) {
    char line[256];
    while (fgets(line, sizeof(line), f)) {
      if (strncmp(line, "model name", 10) == 0 ||
          strncmp(line, "Model name", 10) == 0) {
        char *colon = strchr(line, ':');
        if (colon) {
          colon++;
          while (*colon == ' ' || *colon == '\t')
            colon++;
          size_t slen = strlen(colon);
          if (slen > 0 && colon[slen - 1] == '\n')
            colon[slen - 1] = '\0';
          strncpy(brand, colon, max_len - 1);
          brand[max_len - 1] = '\0';
          fclose(f);
          return;
        }
      }
    }
    fclose(f);
  }
#endif
#if defined(__x86_64__)
  uint32_t regs[12];
  if (__get_cpuid(0x80000002, &regs[0], &regs[1], &regs[2], &regs[3]) &&
      __get_cpuid(0x80000003, &regs[4], &regs[5], &regs[6], &regs[7]) &&
      __get_cpuid(0x80000004, &regs[8], &regs[9], &regs[10], &regs[11])) {
    size_t copy_len = 48 < max_len ? 48 : max_len;
    memcpy(brand, regs, copy_len);
    brand[copy_len - 1] = '\0';
    char *s = brand;
    while (*s == ' ')
      s++;
    if (s != brand)
      memmove(brand, s, strlen(s) + 1);
    return;
  }
#endif
  strncpy(brand, "unknown", max_len - 1);
  brand[max_len - 1] = '\0';
}

static uint32_t get_l1d_size() {
#if defined(__APPLE__)
  uint64_t size = 0;
  size_t len = sizeof(size);
  // P-cores (perflevel0) — what matters for compute-heavy work
  if (sysctlbyname("hw.perflevel0.l1dcachesize", &size, &len, nullptr, 0) == 0)
    return (uint32_t)size;
  // Fallback to generic (returns E-core value on Apple Silicon)
  len = sizeof(size);
  if (sysctlbyname("hw.l1dcachesize", &size, &len, nullptr, 0) == 0)
    return (uint32_t)size;
#endif
#if defined(__linux__)
  for (int idx = 0; idx < 10; idx++) {
    char path[128], buf[32];
    snprintf(path, sizeof(path),
             "/sys/devices/system/cpu/cpu0/cache/index%d/level", idx);
    FILE *f = fopen(path, "r");
    if (!f)
      break;
    int level = 0;
    if (fscanf(f, "%d", &level) != 1) {
      fclose(f);
      continue;
    }
    fclose(f);
    if (level != 1)
      continue;
    snprintf(path, sizeof(path),
             "/sys/devices/system/cpu/cpu0/cache/index%d/type", idx);
    f = fopen(path, "r");
    if (!f)
      continue;
    buf[0] = 0;
    if (fscanf(f, "%31s", buf) != 1) {
      fclose(f);
      continue;
    }
    fclose(f);
    if (strcmp(buf, "Data") != 0)
      continue;
    snprintf(path, sizeof(path),
             "/sys/devices/system/cpu/cpu0/cache/index%d/size", idx);
    f = fopen(path, "r");
    if (!f)
      continue;
    uint32_t size = 0;
    char unit = 0;
    if (fscanf(f, "%u%c", &size, &unit) >= 1) {
      fclose(f);
      if (unit == 'K' || unit == 'k')
        return size * 1024;
      if (unit == 'M' || unit == 'm')
        return size * 1024 * 1024;
      return size;
    }
    fclose(f);
  }
#endif
#if defined(__x86_64__)
  uint32_t eax, ebx, ecx, edx;
  if (__get_cpuid(0x80000005, &eax, &ebx, &ecx, &edx)) {
    uint32_t l1d_kb = (ecx >> 24) & 0xFF;
    if (l1d_kb > 0)
      return l1d_kb * 1024;
  }
  for (uint32_t sub = 0; sub < 10; sub++) {
    if (!__get_cpuid_count(4, sub, &eax, &ebx, &ecx, &edx))
      break;
    uint32_t type = eax & 0x1F;
    if (type == 0)
      break;
    if (type == 1 && ((eax >> 5) & 0x7) == 1) {
      uint32_t ways = ((ebx >> 22) & 0x3FF) + 1;
      uint32_t parts = ((ebx >> 12) & 0x3FF) + 1;
      uint32_t line = (ebx & 0xFFF) + 1;
      uint32_t sets = ecx + 1;
      return ways * parts * line * sets;
    }
  }
#endif
  return 32 * 1024;
}

static void on_exit_handler() {
  printf("total chains: %" PRIu64 "\n", total_chains);
#if CAPTURE_STATS
  printf("new expressions at chain length:\n");
  printf("                   n                       sum              avg"
         "              min              max\n");
  stats_total_num_expressions[start_chain_length] +=
      stats_total_num_expressions[start_chain_length - 1];
  stats_min_num_expressions[start_chain_length] +=
      stats_min_num_expressions[start_chain_length - 1];
  stats_max_num_expressions[start_chain_length] +=
      stats_min_num_expressions[start_chain_length - 1];
  for (uint32_t i = start_chain_length; i < MAX_LENGTH; i++) {
    printf("%2d: %16" PRIu64 " %25" PRIu64 " %16" PRIu64 " %16" PRId32
           " %16" PRIu32 "\n",
           i, stats_num_data_points[i], stats_total_num_expressions[i],
           stats_num_data_points[i] == 0
               ? (uint64_t)0
               : stats_total_num_expressions[i] / stats_num_data_points[i],
           stats_min_num_expressions[i] == UNDEFINED
               ? (uint32_t)0
               : stats_min_num_expressions[i],
           stats_max_num_expressions[i]);
  }
#endif
}

static void signal_handler(int sig) { exit(sig); }

// 00: both plain
#define SEARCH_FUNC search_both_plain
#define UNSEEN_GET(v) UNSEEN_PLAIN_GET(v)
#define UNSEEN_CLEAR(v) UNSEEN_PLAIN_CLEAR(v)
#define UNSEEN_CLEAR_COND(v, c) UNSEEN_PLAIN_CLEAR_COND(v, c)
#define UNSEEN_RESTORE(v) UNSEEN_PLAIN_RESTORE(v)
#define UNSEEN_INIT() UNSEEN_PLAIN_INIT()
#define TARGET_GET(v) TARGET_PLAIN_GET(v)
#define TARGET_INIT() TARGET_PLAIN_INIT()
#include "search-impl.cpp"

// 01: unseen bitset
#define SEARCH_FUNC search_unseen_bitset
#define UNSEEN_GET(v) UNSEEN_BITS_GET(v)
#define UNSEEN_CLEAR(v) UNSEEN_BITS_CLEAR(v)
#define UNSEEN_CLEAR_COND(v, c) UNSEEN_BITS_CLEAR_COND(v, c)
#define UNSEEN_RESTORE(v) UNSEEN_BITS_RESTORE(v)
#define UNSEEN_INIT() UNSEEN_BITS_INIT()
#define TARGET_GET(v) TARGET_PLAIN_GET(v)
#define TARGET_INIT() TARGET_PLAIN_INIT()
#include "search-impl.cpp"

// 10: target bitset
#define SEARCH_FUNC search_target_bitset
#define UNSEEN_GET(v) UNSEEN_PLAIN_GET(v)
#define UNSEEN_CLEAR(v) UNSEEN_PLAIN_CLEAR(v)
#define UNSEEN_CLEAR_COND(v, c) UNSEEN_PLAIN_CLEAR_COND(v, c)
#define UNSEEN_RESTORE(v) UNSEEN_PLAIN_RESTORE(v)
#define UNSEEN_INIT() UNSEEN_PLAIN_INIT()
#define TARGET_GET(v) TARGET_BITS_GET(v)
#define TARGET_INIT() TARGET_BITS_INIT()
#include "search-impl.cpp"

// 11: both bitset
#define SEARCH_FUNC search_both_bitset
#define UNSEEN_GET(v) UNSEEN_BITS_GET(v)
#define UNSEEN_CLEAR(v) UNSEEN_BITS_CLEAR(v)
#define UNSEEN_CLEAR_COND(v, c) UNSEEN_BITS_CLEAR_COND(v, c)
#define UNSEEN_RESTORE(v) UNSEEN_BITS_RESTORE(v)
#define UNSEEN_INIT() UNSEEN_BITS_INIT()
#define TARGET_GET(v) TARGET_BITS_GET(v)
#define TARGET_INIT() TARGET_BITS_INIT()
#include "search-impl.cpp"

// c: combined array (unseen=bit0, target=bit1)
#define SEARCH_FUNC search_combined
#define UNSEEN_GET(v) UNSEEN_COMBINED_GET(v)
#define UNSEEN_CLEAR(v) UNSEEN_COMBINED_CLEAR(v)
#define UNSEEN_CLEAR_COND(v, c) UNSEEN_COMBINED_CLEAR_COND(v, c)
#define UNSEEN_RESTORE(v) UNSEEN_COMBINED_RESTORE(v)
#define UNSEEN_INIT() UNSEEN_COMBINED_INIT()
#define TARGET_GET(v) TARGET_COMBINED_GET(v)
#define TARGET_INIT() TARGET_COMBINED_INIT()
#include "search-impl.cpp"

static const uint16_t target_hash_table[8] = {0x5D40, 0x481C, 0x4921, 0x3EFF,
                                              0x061B, 0x700C, 0x200B, 0xFFFF};

#define TARGET_HASH_GET(v)                                                     \
  (target_hash_table[((uint32_t)((v) * 11) >> 12) & 7] == (uint16_t)(v))
#define TARGET_HASH_INIT() ((void)0)

// h: plain unseen + hash target
#define SEARCH_FUNC search_hash_target
#define UNSEEN_GET(v) UNSEEN_PLAIN_GET(v)
#define UNSEEN_CLEAR(v) UNSEEN_PLAIN_CLEAR(v)
#define UNSEEN_CLEAR_COND(v, c) UNSEEN_PLAIN_CLEAR_COND(v, c)
#define UNSEEN_RESTORE(v) UNSEEN_PLAIN_RESTORE(v)
#define UNSEEN_INIT() UNSEEN_PLAIN_INIT()
#define TARGET_GET(v) TARGET_HASH_GET(v)
#define TARGET_INIT() TARGET_HASH_INIT()
#include "search-impl.cpp"

int main(int argc, char *argv[]) {
  int mode = -1;
  int arg_skip = 0;

  if (argc >= 3 && strcmp(argv[1], "-m") == 0) {
    const char *m = argv[2];
    if (strcmp(m, "c") == 0) {
      mode = 4;
      arg_skip = 2;
    } else if (strcmp(m, "h") == 0) {
      mode = 5;
      arg_skip = 2;
    } else if (strlen(m) == 2 && (m[0] == '0' || m[0] == '1') &&
               (m[1] == '0' || m[1] == '1')) {
      mode = (m[0] - '0') * 2 + (m[1] - '0');
      arg_skip = 2;
    } else {
      fprintf(stderr,
              "Invalid -m argument: %s\n"
              "  Expected 00, 01, 10, or 11\n"
              "  First bit = target_bitset, second bit = unseen_bitset\n",
              m);
      return 1;
    }
  }

  char cpu_brand[128] = {0};
  get_cpu_brand(cpu_brand, sizeof(cpu_brand));
  uint32_t l1d = get_l1d_size();

  printf("CPU: %s\n", cpu_brand);
  printf("L1d cache: %u KB\n", l1d / 1024);

  if (mode < 0) {
    if (l1d > 65536)
      mode = 0;
    else if (l1d > 32768)
      mode = 2;
    else
      mode = 3;
  }

  static const char *mode_names[] = {
      "00 (both plain)",               //
      "01 (unseen bitset)",            //
      "10 (target bitset)",            //
      "11 (both bitset)",              //
      "c (combined array)",            //
      "h (hash target, plain unseen)", //
  };
  printf("Mode: %s%s\n", mode_names[mode], arg_skip ? "" : " (auto)");
  fflush(stdout);

#if !PLAN_MODE
  atexit(on_exit_handler);
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
#endif

  int sa = argc - arg_skip;
  char **sv = argv + arg_skip;

  switch (mode) {
  case 0:
    search_both_plain(sa, sv);
    break;
  case 1:
    search_unseen_bitset(sa, sv);
    break;
  case 2:
    search_target_bitset(sa, sv);
    break;
  case 3:
    search_both_bitset(sa, sv);
    break;
  case 4:
    search_combined(sa, sv);
    break;
  case 5:
    search_hash_target(sa, sv);
    break;
  }

  return 0;
}
