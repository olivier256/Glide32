#include "framebuffer.h"

void fb_init_line_table(int screenHeight) {
  for (int y = 0; y < screenHeight; y++)
    vdp_dram[y] = (uint16_t)((FB_LINE_TABLE_BYTES + y * FB_WIDTH) / 2);
}
