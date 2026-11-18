
#include "main.h"
#include "platform.h"
#include "vdp.h"
#include <stdlib.h>

#define SCREEN_WIDTH 320
#define DEFAULT_SCREEN_HEIGHT 224

int main(int argc, char *argv[]) {
  int running = 1;

  int h = DEFAULT_SCREEN_HEIGHT;
  if (argc > 1) {
    h = atoi(argv[1]);
    if (h <= 0)
      return 1;
  }

  const int screenHeight = h;

  if (!platform_init(SCREEN_WIDTH, screenHeight))
    return 1;

  /* Set Blank Mode */
  *bitmapModeRegister = (*bitmapModeRegister & 0xFFFC);

  /* Line Table Init */
  const int BITMAP_OFFSET = (screenHeight * 2);
  lineTableInit(screenHeight, BITMAP_OFFSET);
  vdp_write_fbcr(vdp_read_fbcr() ^ FBCR_FS);
  lineTableInit(screenHeight, BITMAP_OFFSET);

  /* Set palette */
  vdp_palette[1] = 0x001F;

  /* Set Packed Pixel Mode */
  *bitmapModeRegister = (*bitmapModeRegister & 0xFFFC) | 0x0001;

  /* Main loop */
  int x = 0;
  int y = screenHeight / 2;
  const uint8_t BLACK = 0;
  const uint8_t RED = 1;

  while (running) {
    setPixel(x, y, RED);

    vdp_write_fbcr(vdp_read_fbcr() ^ FBCR_FS);
    running = platform_wait_vsync();
    if (!running) {
      break;
    }

    for (int row = 0; row < screenHeight; row++) {
      for (int col = 0; col < SCREEN_WIDTH; col++) {
        setPixel(col, row, BLACK);
      }
    }

    if (x < SCREEN_WIDTH - 1) {
      x++;
    } else {
      x = 0;
    }
  }

  platform_shutdown();

  return 0;
}

void lineTableInit(const int screenHeight, const int BITMAP_OFFSET) {
  for (int y = 0; y < screenHeight; y++) {
    uint16_t address = (BITMAP_OFFSET / 2) + y * (SCREEN_WIDTH / 2);
    vdp_dram[y] = address;
  }
}

void setPixel(int x, int y, uint8_t c) {
  uint16_t lineAddr_2 = vdp_dram[y];
  uint16_t *word = &vdp_dram[lineAddr_2 + x / 2];
  uint8_t *byte = (uint8_t *)(word + ((x & 1) ? 0 : 1));
  *byte = c;
}
