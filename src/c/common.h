#pragma once
#include <pebble.h>

// Grid parameters struct to reduce function parameter passing
typedef struct {
  int16_t cols;
  int16_t rows;
  int16_t offset_x;
  int16_t offset_y;
  uint8_t cell_size;
  uint8_t full_size;
  uint8_t full_offset;
  uint8_t partial_size;
  uint8_t partial_offset;
  GColor fg_color;
  GColor secondary_color;
} GridParams;

// Shared PRNG (Linear Congruential Generator)
extern uint32_t g_random_seed;

static inline uint32_t prng_next(void) {
  g_random_seed = (g_random_seed * 1103515245 + 12345) & 0x7fffffff;
  return g_random_seed;
}

static inline void prng_seed(uint32_t seed) {
  g_random_seed = seed;
}

// Fixed-point math helpers (8.8 format: 256 = 1.0)
#define FP_ONE 256
#define FP_HALF 128

// Cell states
#define CELL_EMPTY   0
#define CELL_PARTIAL 1
#define CELL_FULL    2
