#include "game.h"
#include "interface.h"
#include "platform.h" /* platform_running() uniquement */
#include "vdp.h"

/* Protocole master <-> slave via les registres COMM */
#define COMM_CMD 0     /* master -> slave */
#define CMD_READY 0x0001 /* le master a fini d'initialiser le VDP */

static void setPixel(int x, int y, uint8_t c);
static void lineTableInit(int screenHeight, int bitmapOffset);
static void clearScreen(int screenHeight, uint16_t c);
static int waitForVBlank(void);
static int waitForDisplay(void);

/* ---- Master ------------------------------------------------------------ */

void master_main(void) {
  const int screenHeight = vdp_screen_height();

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

  /* Le VDP est prêt : on libère le slave */
  comm[COMM_CMD] = CMD_READY;

  /* Main loop */
  int x = 0;
  int y = screenHeight / 2;
  const uint8_t BLACK = 0;
  const uint8_t RED = 1;

  uint16_t blackAs16Bits = (BLACK << 8) | BLACK;

  for (;;) {
    setPixel(x, y, RED);

    if (x < VDP_SCREEN_WIDTH - 1)
      x++;
    else
      x = 0;

    /* Garde-fou : la demande de flip doit tomber hors VBlank */
    if (!waitForDisplay())
      return;

    vdp_write_fbcr(vdp_read_fbcr() ^ FBCR_FS);
    if (!waitForVBlank())
      return;

    /* Pendant le VBlank : on prépare le back buffer */
    clearScreen(screenHeight, blackAs16Bits);
  }
}

/* ---- Slave ------------------------------------------------------------- */

void slave_main(void) {
  int x = 50;
  int y = 50;
  const uint8_t GREEN = 2;

  /* Attendre que le master ait initialisé le VDP (tables de lignes) */
  while (comm[COMM_CMD] != CMD_READY)
    if (!platform_running())
      return;

  if (!waitForVBlank())
    return;

  for (;;) {
    /* Le master efface pendant le VBlank : on attend qu'il ait fini */
    if (!waitForDisplay())
      return;

    setPixel(x, y, GREEN);
    setPixel(x, y + 1, GREEN);

    if (x < VDP_SCREEN_WIDTH - 1)
      x++;
    else
      x = 0;

    if (!waitForVBlank())
      return;
  }
}

/* ---- Attentes VDP (renvoient 0 si l'hôte demande l'arrêt) -------------- */

/* Attend le début du VBlank (VBLK passe à 1) */
static int waitForVBlank(void) {
  while ((vdp_read_fbcr() & FBCR_VBLK) == 0)
    if (!platform_running())
      return 0;
  return 1;
}

/* Attend la fin du VBlank, retour à l'affichage (VBLK passe à 0) */
static int waitForDisplay(void) {
  while ((vdp_read_fbcr() & FBCR_VBLK) != 0)
    if (!platform_running())
      return 0;
  return 1;
}

/* ---- Dessin ------------------------------------------------------------ */

static void lineTableInit(int screenHeight, int bitmapOffset) {
  for (int y = 0; y < screenHeight; y++) {
    uint16_t address = (bitmapOffset / 2) + y * (VDP_SCREEN_WIDTH / 2);
    vdp_dram[y] = address;
  }
}

static void setPixel(int x, int y, uint8_t c) {
  uint16_t lineAddr_2 = vdp_dram[y];
  uint16_t *word = &vdp_dram[lineAddr_2 + x / 2];

  if (x & 1)
    *word = (*word & 0xFF00) | c;
  else
    *word = (*word & 0x00FF) | ((uint16_t)c << 8);
}

static void clearScreen(int screenHeight, uint16_t c) {
  int count = screenHeight * (VDP_SCREEN_WIDTH / 2);
  uint16_t *p = vdp_dram + screenHeight;

  for (int i = 0; i < count; i++) {
    *p++ = c;
  }
}
