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

  /* SLI visible (lignes du master en rouge, du slave en vert) :
     grSliDebug(1, RED, GREEN); */

  /* Z-buffer avec ooz (1/z) : le plus grand est le plus proche, d'où
     GR_CMP_GREATER et un effacement à 0 (exemple 7.1 du guide Glide). */
  grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);
  grDepthBufferFunction(GR_CMP_GREATER);
  grDepthMask(1);

  /* Deux triangles qui se traversent :
     - le rouge est proche à gauche et s'éloigne vers la droite ;
     - le vert est lointain à gauche, et sa profondeur à droite varie,
       ce qui déplace la ligne d'intersection. */
  GrVertex r0 = {GR_FIXED(30), GR_FIXED(30), GR_DEPTH(60000)};
  GrVertex r1 = {GR_FIXED(30), GR_FIXED(190), GR_DEPTH(60000)};
  GrVertex r2 = {GR_FIXED(290), GR_FIXED(110), GR_DEPTH(10000)};

  GrVertex g0 = {GR_FIXED(290), GR_FIXED(50), GR_DEPTH(40000)};
  GrVertex g1 = {GR_FIXED(290), GR_FIXED(210), GR_DEPTH(40000)};
  GrVertex g2 = {GR_FIXED(30), GR_FIXED(130), GR_DEPTH(10000)};

  const GrDepth step = GR_DEPTH(200);
  int closer = 1;

  while (platform_running()) {
    grBufferClear(BLACK, 0);

    grConstantColorValue(RED);
    grDrawTriangle(&r0, &r1, &r2);

    grConstantColorValue(GREEN);
    grDrawTriangle(&g0, &g1, &g2);

    grBufferSwap(1);

    /* Le côté droit du vert avance et recule entre 15000 et 60000 */
    if (closer)
      g0.ooz += step;
    else
      g0.ooz -= step;
    g1.ooz = g0.ooz;
    if (g0.ooz <= GR_DEPTH(15000) || g0.ooz >= GR_DEPTH(60000))
      closer = !closer;
  }
}

/* ---- Slave ------------------------------------------------------------- */

void slave_main(void) {
  interface_clear_vint();
  interface_write_intmask(interface_read_intmask() | INTMASK_V);

  /* Le slave devient la seconde carte du SLI */
  grSlaveLoop();
}
