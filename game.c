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

/* ---- Projection -----------------------------------------------------------
   Glide ne projette pas : c'est au jeu de passer de la 3D à l'écran.
   Repère de la caméra : x vers la droite, y vers le haut, z vers l'avant
   (la caméra est à l'origine et regarde vers les z positifs).
   Tout est en 16.16. */

typedef struct {
  GrFixed x, y, z;
} Vec3;

/* Distance focale en pixels : 160 donne 90 degrés d'angle horizontal */
#define FOCAL 160

/* Plan proche : ooz vaut 65535 à cette distance. Pas encore de découpage :
   tous les points doivent rester à z >= NEAR_Z. */
#define NEAR_Z GR_FIXED(2)

static int screenHeight;

static void project(const Vec3 *v, GrVertex *out) {
  const int64_t z = v->z;

  /* x_écran = centre + FOCAL * x / z ; y inversé (l'écran descend) */
  out->x = GR_FIXED(VDP_SCREEN_WIDTH / 2) +
           (GrFixed)(((int64_t)v->x * FOCAL * GR_FIXED_ONE) / z);
  out->y = GR_FIXED(screenHeight / 2) -
           (GrFixed)(((int64_t)v->y * FOCAL * GR_FIXED_ONE) / z);

  /* ooz = 65535 * NEAR_Z / z, en 16.16 */
  int64_t ooz = ((int64_t)65535 * NEAR_Z * GR_FIXED_ONE) / z;
  if (ooz > (int64_t)GR_DEPTH(65535))
    ooz = GR_DEPTH(65535);
  out->ooz = (GrDepth)ooz;
}

/* ---- Master ------------------------------------------------------------ */

void master_main(void) {
  /* VDP, FIFO, et libération du slave */
  grSstWinOpen();
  screenHeight = vdp_screen_height();

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

  /* Deux triangles en 3D qui se traversent :
     - le rouge : côté vertical AB à gauche et proche, sommet C au fond ;
     - le vert : côté AB à droite, qui avance et recule en z, sommet C au
       fond à gauche. Son côté AB est incliné en profondeur (B est 2 plus
       loin que A) : sinon, les deux plans contiendraient la verticale, et
       leur intersection serait une droite verticale. */
  const Vec3 redA = {GR_FIXED(-3), GR_FIXED(2), GR_FIXED(4)};
  const Vec3 redB = {GR_FIXED(-3), GR_FIXED(-2), GR_FIXED(4)};
  const Vec3 redC = {GR_FIXED(3), GR_FIXED(0), GR_FIXED(10)};

  Vec3 greenA = {GR_FIXED(5) / 2, GR_FIXED(2), GR_FIXED(3)};
  Vec3 greenB = {GR_FIXED(5) / 2, GR_FIXED(-2), GR_FIXED(5)};
  const Vec3 greenC = {GR_FIXED(-3), GR_FIXED(0), GR_FIXED(10)};

  GrFixed dz = GR_FIXED_ONE / 32; /* par image */

  while (platform_running()) {
    GrVertex a, b, c;

    grBufferClear(BLACK, 0);

    grConstantColorValue(RED);
    project(&redA, &a);
    project(&redB, &b);
    project(&redC, &c);
    grDrawTriangle(&a, &b, &c);

    grConstantColorValue(GREEN);
    project(&greenA, &a);
    project(&greenB, &b);
    project(&greenC, &c);
    grDrawTriangle(&a, &b, &c);

    grBufferSwap(1);

    /* Le côté AB du vert va et vient : A entre z = 3 et z = 7 */
    greenA.z += dz;
    greenB.z = greenA.z + GR_FIXED(2);
    if (greenA.z <= GR_FIXED(3) || greenA.z >= GR_FIXED(7))
      dz = -dz;
  }
}

/* ---- Slave ------------------------------------------------------------- */

void slave_main(void) {
  interface_clear_vint();
  interface_write_intmask(interface_read_intmask() | INTMASK_V);

  /* Le slave devient la seconde carte du SLI */
  grSlaveLoop();
}
