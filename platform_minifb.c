#include "platform.h"
#include "vdp.h"

#include <MiniFB.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

static struct mfb_window *_window = NULL;

static uint32_t *_display = NULL;

static int _width = 0;
static int _height = 0;

int platform_init(int width, int height) {
  _width = width;
  _height = height;

  _window = mfb_open("VDP", width, height);

  if (_window == NULL)
    return 0;

  _display = malloc((size_t)width * height * sizeof(uint32_t));

  if (_display == NULL) {
    platform_shutdown();
    return 0;
  }

  return 1;
}

int display(void);
void displayPackedPixelMode();
uint8_t read_vram_byte(uint32_t address);
uint32_t get_palette_color(uint8_t index);
static void wait_vblank(void);

int platform_wait_vsync(void) {
  if (_window == NULL)
    return 0;

  if (!mfb_wait_sync(_window))
    return 0;

  vdp_begin_vblank();

  wait_vblank();

  vdp_end_vblank();

  return display();
}

static void wait_vblank(void) {
  struct timespec duration = {0, 2420000};
  nanosleep(&duration, NULL);
}

int display(void) {
  if (_window == NULL || _display == NULL)
    return 0;

  switch (*bitmapModeRegister & 3) {
  case 0: // BLANK_MODE
    break;
  case 1: // PACKED_PIXEL_MODE
    displayPackedPixelMode();
    break;
  default: // DIRECT_COLOR_MODE, RUN_LENGTH_MODE
    break;
  }

  mfb_update_state state = mfb_update(_window, _display);

  if (state == MFB_STATE_EXIT) {
    _window = NULL;
    return 0;
  }
  return state == MFB_STATE_OK;
}

void displayPackedPixelMode(void) {
  for (int row = 0; row < _height; row++) {
    uint32_t address = (uint32_t)displayFrameBuffer[row] * 2;

    for (int col = 0; col < _width; col++) {
      uint8_t index = read_vram_byte(address + col);
      _display[row * _width + col] = get_palette_color(index);
    }
  }
}

uint8_t read_vram_byte(uint32_t address) {
  uint16_t word = displayFrameBuffer[address / 2];

  if ((address & 1) == 0)
    return (uint8_t)(word >> 8);

  return (uint8_t)(word & 0x00FF);
}

uint32_t get_palette_color(uint8_t index) {
  uint16_t color = vdp_palette[index];

  uint8_t r = color & 0x1F;
  uint8_t g = (color >> 5) & 0x1F;
  uint8_t b = (color >> 10) & 0x1F;

  r = (r << 3) | (r >> 2);
  g = (g << 3) | (g >> 2);
  b = (b << 3) | (b >> 2);

  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void platform_shutdown(void) {
  free(_display);

  _display = NULL;

  if (_window != NULL) {
    mfb_close(_window);
    _window = NULL;
  }
}
