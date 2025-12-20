#pragma once
#include <pebble.h>

// Draw matrix falling animation
void draw_matrix_animation(GContext *ctx, float progress, float fade,
                           int grid_cols, int grid_rows,
                           int grid_offset_x, int grid_offset_y,
                           int cell_size, int full_size, int full_offset,
                           int partial_size, int partial_offset,
                           GColor fg_color, GColor secondary_color);
