#include "game.h"
#include "glide.h"
#include "interface.h"
#include "platform.h" /* platform_running() uniquement */
#include "tnl.h"
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

  /* Projection : l'œil regarde vers les z négatifs. Plan proche à 300,
     fenêtre de 240 x (3 * h / 4) sur ce plan : la focale vaut 400 pixels,
     si bien qu'à 400 de l'œil, une unité fait exactement un pixel. */
  const int h = vdp_screen_height();
  glViewport(0, 0, VDP_SCREEN_WIDTH, h);
  glFrustumx(-FIXED(120), FIXED(120), -FIXED(3 * h) / 8, FIXED(3 * h) / 8,
             FIXED(300), FIXED(1000));

  /* Les triangles de l'énoncé, donnés en pixels (x, y vers le bas) à
     l'écran, deviennent : x - 160, 112 - y (h = 224), z = -400.
     Au départ, chacun s'affiche exactement aux coordonnées de l'énoncé. */
  GLangle redAngle = 0;
  GLangle greenAngle = 0;

  while (platform_running()) {
    grBufferClear(BLACK, 0);

    /* Rouge : A (100, 50), B (100, 150), C (200, 100) ; rotation autour
       d'un axe parallèle à y, en x = 150, z = profondeur de la scène. */
    glLoadIdentity();
    glTranslatex(FIXED(-10), 0, FIXED(-400)); /* 3. axe remis en place */
    glRotatex(redAngle, 0, FIXED_ONE, 0);     /* 2. rotation autour de y */
    glTranslatex(FIXED(10), 0, FIXED(400));   /* 1. axe ramené à l'origine */

    glIndexub(RED);
    glBegin(GL_TRIANGLES);
    glVertex3x(FIXED(-60), FIXED(62), FIXED(-400));
    glVertex3x(FIXED(-60), FIXED(-38), FIXED(-400));
    glVertex3x(FIXED(40), FIXED(12), FIXED(-400));
    glEnd();

    /* Vert : D (300, 30), E (300, 190), F (125, 170) ; rotation autour
       d'un axe parallèle à x, en y = 110. */
    glLoadIdentity();
    glTranslatex(0, FIXED(2), FIXED(-400));
    glRotatex(greenAngle, FIXED_ONE, 0, 0);
    glTranslatex(0, FIXED(-2), FIXED(400));

    glIndexub(GREEN);
    glBegin(GL_TRIANGLES);
    glVertex3x(FIXED(140), FIXED(82), FIXED(-400));
    glVertex3x(FIXED(140), FIXED(-78), FIXED(-400));
    glVertex3x(FIXED(-35), FIXED(-58), FIXED(-400));
    glEnd();

    grBufferSwap(1);

    redAngle += GL_DEG(2);   /* un tour en 3 secondes */
    greenAngle += GL_DEG(1); /* un tour en 6 secondes */
  }
}

/* ---- Slave ------------------------------------------------------------- */

void slave_main(void) {
  interface_clear_vint();
  interface_write_intmask(interface_read_intmask() | INTMASK_V);

  /* Le slave devient la seconde carte du SLI */
  grSlaveLoop();
}
