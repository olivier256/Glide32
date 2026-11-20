/* =========================================================================
   Banc de mesure : combien de triangles par seconde ?

   Remplace game.c (même points d'entrée). Outil propre à l'hôte : il
   affiche ses résultats avec printf, ce qu'un jeu ne fait pas.

   L'horloge est celle de la console : le compteur de V_INT du master
   (60 par seconde). Chaque test dessine sans relâche pendant 2 secondes,
   sans échange de buffers (pas d'attente du VBlank), puis grFinish()
   attend que le slave ait fini : le temps compté inclut tout le travail
   des deux CPU.

   ATTENTION : on mesure le Mac, pas une 32X. Les chiffres servent à
   comparer les étages entre eux (coût fixe, remplissage, z-buffer, T&L),
   pas à prévoir les performances d'un SH2.
   ========================================================================= */

#include "game.h"
#include "glide.h"
#include "interface.h"
#include "platform.h" /* platform_running() uniquement */
#include "tnl.h"
#include "vdp.h"

#include <stdio.h>

#define BLACK 0
#define RED 1
#define GREEN 2
#define WHITE 3

#define TEST_FRAMES 120 /* 2 secondes */
#define BATCH 16        /* triangles entre deux lectures de l'horloge */

static volatile unsigned long masterVInts = 0;

void master_vint(void) {
  interface_clear_vint();
  masterVInts++;
}

void slave_vint(void) { interface_clear_vint(); }

/* ---- Les tests ----------------------------------------------------------- */

typedef enum { PATH_GLIDE, PATH_MINIGL } Path;

typedef struct {
  const char *name;
  Path path;
  int size;     /* côté de l'angle droit, en pixels ; 0 : hors écran */
  int zbuffer;
} Test;

static const Test tests[] = {
    {"Glide,  hors écran (coût fixe)", PATH_GLIDE, 0, 0},
    {"Glide,  côté 10  (~50 px)", PATH_GLIDE, 10, 0},
    {"Glide,  côté 32  (~512 px)", PATH_GLIDE, 32, 0},
    {"Glide,  côté 100 (~5000 px)", PATH_GLIDE, 100, 0},
    {"Glide,  côté 32  + z-buffer", PATH_GLIDE, 32, 1},
    {"Glide,  côté 100 + z-buffer", PATH_GLIDE, 100, 1},
    {"MiniGL, hors écran (T&L + coût fixe)", PATH_MINIGL, 0, 0},
    {"MiniGL, côté 32  (~512 px)", PATH_MINIGL, 32, 0},
    {"MiniGL, côté 32  + z-buffer", PATH_MINIGL, 32, 1},
};

#define TEST_COUNT ((int)(sizeof(tests) / sizeof(tests[0])))

/* Coin du triangle n° i : on le promène sur l'écran */
static void corner(int i, int size, int screenHeight, int *x, int *y) {
  if (size == 0) { /* sous l'écran : aucune ligne à tracer */
    *x = (i * 53) % 300;
    *y = screenHeight + 10;
    return;
  }
  *x = (i * 53) % (VDP_SCREEN_WIDTH - size);
  *y = (i * 29) % (screenHeight - size);
}

/* Dessine BATCH triangles par le chemin demandé */
static void drawBatch(const Test *t, unsigned long first, int screenHeight) {
  const int s = t->size ? t->size : 10;

  if (t->path == PATH_GLIDE) {
    for (int i = 0; i < BATCH; i++) {
      int x, y;
      corner((int)(first + i), t->size, screenHeight, &x, &y);

      GrVertex a = {FIXED(x), FIXED(y), GR_DEPTH(30000)};
      GrVertex b = {FIXED(x + s), FIXED(y), GR_DEPTH(30000)};
      GrVertex c = {FIXED(x), FIXED(y + s), GR_DEPTH(30000)};
      grDrawTriangle(&a, &b, &c);
    }
  } else {
    /* Mêmes triangles, décrits en 3D à z = -400 (un pixel par unité),
       avec une vraie matrice : une rotation nulle n'en coûte pas moins. */
    glLoadIdentity();
    glRotatex(0, 0, FIXED_ONE, 0);

    glBegin(GL_TRIANGLES);
    for (int i = 0; i < BATCH; i++) {
      int x, y;
      corner((int)(first + i), t->size, screenHeight, &x, &y);

      const GLfixed x0 = FIXED(x - VDP_SCREEN_WIDTH / 2);
      const GLfixed y0 = FIXED(screenHeight / 2 - y);
      glVertex3x(x0, y0, FIXED(-400));
      glVertex3x(x0 + FIXED(s), y0, FIXED(-400));
      glVertex3x(x0, y0 - FIXED(s), FIXED(-400));
    }
    glEnd();
  }
}

/* Attend le prochain V_INT (début d'une période de 1/60 s) */
static void waitTick(void) {
  const unsigned long start = masterVInts;
  while (masterVInts == start && platform_running())
    ;
}

static void runTest(const Test *t, int screenHeight) {
  if (t->zbuffer) {
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);
    grDepthBufferFunction(GR_CMP_GEQUAL); /* même profondeur : tout passe et s'écrit */
  } else {
    grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
  }
  grBufferClear(BLACK, 0);
  grFinish();

  waitTick();
  const unsigned long t0 = masterVInts;
  unsigned long count = 0;

  while (masterVInts - t0 < TEST_FRAMES && platform_running()) {
    drawBatch(t, count, screenHeight);
    count += BATCH;
  }
  grFinish();
  const unsigned long frames = masterVInts - t0;

  if (!platform_running() || frames == 0)
    return;

  const double seconds = frames / 60.0;
  const double triPerSecond = count / seconds;
  const double pixels = t->size ? t->size * t->size / 2.0 : 0.0;

  printf("%-40s %10.0f tri/s  %8.0f tri/image  %7.1f Mpixels/s\n", t->name,
         triPerSecond, triPerSecond / 60.0, triPerSecond * pixels / 1e6);
  fflush(stdout);
}

/* ---- Master ------------------------------------------------------------ */

void master_main(void) {
  grSstWinOpen();
  const int h = vdp_screen_height();

  vdp_palette[BLACK] = 0x0000;
  vdp_palette[RED] = 0x001F;
  vdp_palette[GREEN] = 0x03E0;
  vdp_palette[WHITE] = 0x7FFF;

  interface_clear_vint();
  interface_write_intmask(interface_read_intmask() | INTMASK_V);

  glViewport(0, 0, VDP_SCREEN_WIDTH, h);
  glFrustumx(-FIXED(120), FIXED(120), -FIXED(3 * h) / 8, FIXED(3 * h) / 8,
             FIXED(300), FIXED(1000));

  /* Les lignes du master en rouge, celles du slave en vert : on voit
     travailler les deux CPU pendant les tests */
  grSliDebug(1, RED, GREEN);

  printf("\nBanc de mesure : %d tests de %d s (horloge : V_INT)\n\n", TEST_COUNT,
         TEST_FRAMES / 60);
  for (int i = 0; i < TEST_COUNT && platform_running(); i++)
    runTest(&tests[i], h);
  printf("\nTerminé. Fermez la fenêtre pour quitter.\n");
  fflush(stdout);

  /* Affiche la dernière image, puis attend la fermeture */
  grBufferSwap(1);
  while (platform_running())
    ;
}

/* ---- Slave ------------------------------------------------------------- */

void slave_main(void) {
  interface_clear_vint();
  interface_write_intmask(interface_read_intmask() | INTMASK_V);
  grSlaveLoop();
}
