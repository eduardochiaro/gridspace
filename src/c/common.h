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
#define FLOAT_TO_FP(f) ((int16_t)((f) * FP_ONE))
#define FP_TO_INT(fp) ((fp) >> 8)
#define FP_MUL(a, b) (((a) * (b)) >> 8)

// Cell states (2 bits each for packing)
#define CELL_EMPTY   0
#define CELL_PARTIAL 1
#define CELL_FULL    2

// Get 2-bit cell value from packed array (4 cells per byte)
static inline uint8_t get_packed_cell(const uint8_t *packed, int index) {
  int byte_idx = index >> 2;  // index / 4
  int bit_pos = (index & 3) << 1;  // (index % 4) * 2
  return (packed[byte_idx] >> bit_pos) & 0x03;
}
