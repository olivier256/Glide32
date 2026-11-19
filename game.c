#include "game.h"
#include "bigendian.h"
#include "interface.h"
#include "platform.h" /* platform_running() uniquement */
#include "vdp.h"

/* Protocole master <-> slave via les registres COMM */
#define COMM_CMD 0       /* master -> slave */
#define CMD_READY 0x0001 /* le master a fini d'initialiser le VDP */

static void setPixel(int x, int y, uint8_t c);
static void setTwoPixels(int x, int y, uint16_t clcr);
static void lineTableInit(int screenHeight, int bitmapOffset);
static void clearScreen(int screenHeight, uint16_t c);
static int waitForMasterVInt(void);
static int waitForSlaveVInt(void);
static int waitForVBlank(void);
static int waitForDisplay(void);

/* ---- Interruptions -----------------------------------------------------
   Chaque routine V_INT acquitte l'interruption et compte les VBlank de son
   CPU. Un compteur qui avance ne peut pas être raté, contrairement au bit
   VBLK, qui retombe à 0 : c'est tout l'intérêt de V_INT. */

static volatile unsigned long masterVInts = 0;
static volatile unsigned long slaveVInts = 0;

void master_vint(void) {
  interface_clear_vint();
  masterVInts++;
}

void slave_vint(void) {
  interface_clear_vint();
  slaveVInts++;
}

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

  /* V_INT : acquitter une éventuelle V_INT déjà mémorisée, puis démasquer */
  interface_clear_vint();
  interface_write_intmask(interface_read_intmask() | INTMASK_V);

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

  interface_clear_vint();
  interface_write_intmask(interface_read_intmask() | INTMASK_V);

  for (;;) {
    /* Début du VBlank, puis fin : le master a fini d'effacer */
    if (!waitForVBlank())
      return;
    if (!waitForDisplay())
      return;

    setPixel(x, y, GREEN);
    setPixel(x, y + 1, GREEN);

    if (x < VDP_SCREEN_WIDTH - 1)
      x++;
    else
      x = 0;
  }
}

/* ---- Attentes (renvoient 0 si l'hôte demande l'arrêt) ------------------ */

/* Attend la prochaine V_INT de ce CPU (son compteur avance) */
static int waitForMasterVInt(void) {
  unsigned long start = masterVInts;
  while (masterVInts == start)
    if (!platform_running())
      return 0;
  return 1;
}

static int waitForSlaveVInt(void) {
  unsigned long start = slaveVInts;
  while (slaveVInts == start)
    if (!platform_running())
      return 0;
  return 1;
}

/* Attend le VBlank, retour à l'affichage (VBLK passe à 1) */
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
  uint8_t *vdpDramAsByte = (uint8_t *)vdp_dram;
  int lineAddr = vdp_dram[y];
  *(vdpDramAsByte + lineAddr * 2 + x) = c;
}

/* Écrit deux pixels d'un coup (x pair) : clcr = pixel gauche en poids fort,
   pixel droit en poids faible, comme MOV.W sur 32X. */
static void setTwoPixels(int x, int y, uint16_t clcr) {
  int lineAddr = vdp_dram[y];
  vdp_dram[lineAddr + x / 2] = BE16(clcr);
}

static void clearScreen(int screenHeight, uint16_t c) {
  int count = screenHeight * (VDP_SCREEN_WIDTH / 2);
  uint16_t *p = vdp_dram + screenHeight;

  for (int i = 0; i < count; i++) {
    *p++ = c;
  }
}
