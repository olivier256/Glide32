
#include "platform.h"
#include "vdp.h"
#include <stdlib.h>

#define SCREEN_WIDTH 320
#define DEFAULT_SCREEN_HEIGHT 224

void setPixel(int x, int y, uint8_t c);
void lineTableInit(const int screenHeight, const int BITMAP_OFFSET);
void clearScreen(int screenHeight, uint16_t c);

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

  const int bitmapOffset = (screenHeight * 2);
  lineTableInit(screenHeight, bitmapOffset);
  vdp_write_fbcr(vdp_read_fbcr() ^ FBCR_FS);
  lineTableInit(screenHeight, bitmapOffset);

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

    uint16_t blackAs16Bits = (BLACK << 8) | BLACK;
    clearScreen(screenHeight, blackAs16Bits);

    if (x < SCREEN_WIDTH - 1) {
      x++;
    } else {
      x = 0;
    }
  }

  platform_shutdown();

  return 0;
}

void lineTableInit(const int screenHeight, const int bitmapOffset) {
  for (int y = 0; y < screenHeight; y++) {
    uint16_t address = (bitmapOffset / 2) + y * (SCREEN_WIDTH / 2);
    vdp_dram[y] = address;
  }
}

void setPixel(int x, int y, uint8_t c) {
  uint16_t lineAddr_2 = vdp_dram[y];
  uint16_t *word = &vdp_dram[lineAddr_2 + x / 2];
  uint8_t *byte = (uint8_t *)(word + ((x & 1) ? 0 : 1));
  *byte = c;
}

void clearScreen(int screenHeight, uint16_t c) {
  int count = screenHeight * (SCREEN_WIDTH / 2);
  uint16_t *p = vdp_dram + screenHeight;

  for (int i = 0; i < count; i++) {
    *p++ = c;
  }
}