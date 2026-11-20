#include "game.h"
#include "glide.h"
#include "interface.h"
#include "platform.h" /* platform_running() uniquement */
#include "vdp.h"

/* ---- Palette ----------------------------------------------------------- */

#define BLACK 0
#define RED 1
#define GREEN 2
#define WHITE 3

/* ---- Interruptions -----------------------------------------------------
   Chaque routine V_INT acquitte l'interruption et compte les VBlank de son
   CPU. Le compteur servira d'horloge au jeu. */

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
  /* VDP, FIFO, et libération du slave */
  grSstWinOpen();

  /* Palette (BGR 5:5:5) */
  vdp_palette[BLACK] = 0x0000;
  vdp_palette[RED] = 0x001F;
  vdp_palette[GREEN] = 0x03E0;
  vdp_palette[WHITE] = 0x7FFF;

  /* V_INT : acquitter une éventuelle V_INT déjà mémorisée, puis démasquer */
  interface_clear_vint();
  interface_write_intmask(interface_read_intmask() | INTMASK_V);

  /* SLI visible : lignes du master en rouge, du slave en vert */
  grSliDebug(1, RED, GREEN);

  /* Un triangle dont le sommet du bas va et vient, au quart de pixel */
  GrVertex a = {GR_FIXED(160), GR_FIXED(20)};
  GrVertex b = {GR_FIXED(60), GR_FIXED(200)};
  GrVertex c = {GR_FIXED(260), GR_FIXED(180)};
  GrFixed dx = GR_FIXED_ONE / 4;

  while (platform_running()) {
    grBufferClear(BLACK, 0);

    grConstantColorValue(WHITE);
    grDrawTriangle(&a, &b, &c);

    grBufferSwap(1);

    b.x += dx;
    if (b.x <= GR_FIXED(20) || b.x >= GR_FIXED(300))
      dx = -dx;
  }
}

/* ---- Slave ------------------------------------------------------------- */

void slave_main(void) {
  interface_clear_vint();
  interface_write_intmask(interface_read_intmask() | INTMASK_V);

  /* Le slave devient la seconde carte du SLI */
  grSlaveLoop();
}
