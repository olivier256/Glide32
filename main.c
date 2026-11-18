
#include "platform.h"
#include "vdp.h"
#include <stdlib.h>

#define SCREEN_WIDTH 320
#define DEFAULT_SCREEN_HEIGHT 224

/* Protocole master <-> slave via les registres COMM */
#define COMM_CMD 0 /* master -> slave : CMD_QUIT pour arreter le slave */
#define CMD_QUIT 0xFFFF

static void comm_write(int reg, uint16_t value) {
  platform_sync(); /* les pixels ecrits avant doivent etre visibles */
  comm_regs[reg] = value;
}

static uint16_t comm_read(int reg) {
  uint16_t value = comm_regs[reg];
  platform_sync(); /* rien ne doit etre lu/ecrit avant ce registre */
  return value;
}

void slave_main(void);
void setPixel(int x, int y, uint8_t c);
void lineTableInit(const int screenHeight, const int BITMAP_OFFSET);
void clearScreen(int screenHeight, uint16_t c);
static void waitForVBlank();
static void waitForDisplay();

#include <stdio.h>
#include <time.h>

static double elapsed_ms(struct timespec start, struct timespec end) {
  return (end.tv_sec - start.tv_sec) * 1000.0 +
         (end.tv_nsec - start.tv_nsec) / 1000000.0;
}

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

  platform_start_slave(slave_main);
  /* Set Blank Mode */
  *bitmapModeRegister = (*bitmapModeRegister & 0xFFFC);

  const int bitmapOffset = (screenHeight * 2);
  lineTableInit(screenHeight, bitmapOffset);
  vdp_write_fbcr(vdp_read_fbcr() ^ FBCR_FS);
  lineTableInit(screenHeight, bitmapOffset);

  /* Set palette */
  vdp_palette[1] = 0x001F;
  vdp_palette[2] = 0x03E0;

  /* Set Packed Pixel Mode */
  *bitmapModeRegister = (*bitmapModeRegister & 0xFFFC) | 0x0001;

  /* Main loop */
  int x = 0;
  int y = screenHeight / 2;
  const uint8_t BLACK = 0;
  const uint8_t RED = 1;

  uint16_t blackAs16Bits = (BLACK << 8) | BLACK;

  while (running) {

    setPixel(x, y, RED);

    if (x < SCREEN_WIDTH - 1)
      x++;
    else
      x = 0;

    // Garde-fou si on est tjs ds le VBlank de la boucle précédente
    waitForDisplay();

    vdp_write_fbcr(vdp_read_fbcr() ^ FBCR_FS);
    if (!platform_wait_vsync())
      break;

    /* Pendant le VBlank : on prépare le back buffer */
    clearScreen(screenHeight, blackAs16Bits);
  }

  comm_write(COMM_CMD, CMD_QUIT);
  platform_join_slave();

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

  if (x & 1)
    *word = (*word & 0xFF00) | c;
  else
    *word = (*word & 0x00FF) | ((uint16_t)c << 8);
}

void clearScreen(int screenHeight, uint16_t c) {
  int count = screenHeight * (SCREEN_WIDTH / 2);
  uint16_t *p = vdp_dram + screenHeight;

  for (int i = 0; i < count; i++) {
    *p++ = c;
  }
}

static void waitForVBlank() {
  while ((vdp_read_fbcr() & FBCR_VBLK) == 0)
    ;
}
static void waitForDisplay() {
  while ((vdp_read_fbcr() & FBCR_VBLK) != 0)
    ;
}

void slave_main(void) {
  int x = 50;
  int y = 50;
  const uint8_t GREEN = 2;

  waitForVBlank();
  for (;;) {
    while ((vdp_read_fbcr() & FBCR_VBLK) != 0) {
      if (comm_read(COMM_CMD) == CMD_QUIT)
        return;
    }

    setPixel(x, y, GREEN);
    setPixel(x, y + 1, GREEN);

    while ((vdp_read_fbcr() & FBCR_VBLK) == 0) {
      if (comm_read(COMM_CMD) == CMD_QUIT)
        return;
    }

    if (x < SCREEN_WIDTH - 1)
      x++;
    else
      x = 0;
  }
}
