/* =========================================================================
   AMB96 : deux plans de tiles qui défilent indépendamment (A en haut,
   B en bas, séparés par une ligne mobile), un grand sprite et quatre
   coins arrondis, dessinés par les deux SH2.

   Déroulement d'une image :

     Master                               Slave
     ------                               -----
     lit la manette (COMM_PAD)
     calcule la scène
     publie la scène, COMM_FRAME = n  --> lit la scène
     dessine les lignes paires            dessine les lignes impaires
     attend COMM_SLAVE_DONE = n      <--  COMM_SLAVE_DONE = n
     demande l'échange de frame buffer
     attend qu'il soit fait (VBlank)
     image suivante                       attend COMM_FRAME = n + 1

   Le Slave ne commence jamais avant l'échange : il attend le COMM_FRAME
   suivant, que le Master n'écrit qu'une fois l'échange fait.

   Commandes (le contenu suit la flèche) :
     Haut/Bas           ligne de séparation
     A + flèches        défilement du plan A
     B + flèches        défilement du plan B
     Start + flèches    déplace le sprite
     C (maintenu)       plus vite
   ========================================================================= */

#include "assets_types.h"
#include "framebuffer.h"
#include "game.h"
#include "interface.h"
#include "plane.h"
#include "platform.h"
#include "shared.h"
#include "sprite.h"
#include "vdp.h"

#include "amb96_assets.h" /* généré par bmp2c depuis assets/assets.txt */

#define SPEED_SLOW 1 /* pixels par image */
#define SPEED_FAST 4

/* Scène de l'image en cours, publiée par le Master (voir shared.h) */
static Scene sharedScene;

static void initVDP(int screenHeight);
static void updateScene(Scene *scene, unsigned pad, int screenHeight);
static void drawScene(const Scene *scene, int cpu, int screenHeight);
static int flipFrameBuffer(void);

/* ---- Interruptions : non utilisées (V_INT reste masquée) ---------------- */

void master_vint(void) { interface_clear_vint(); }
void slave_vint(void) { interface_clear_vint(); }

/* ---- Master ------------------------------------------------------------- */

void master_main(void) {
  const int screenHeight = vdp_screen_height();

  initVDP(screenHeight);

  /* Valeurs de départ d'AMB96.asm */
  Scene scene = {
      .splitLine = 80,
      .scrollAX = 300,
      .scrollAY = 336,
      .scrollBX = 300,
      .scrollBY = 336,
      .sonicX = (FB_WIDTH - 128) / 2,
      .sonicY = 40,
  };

  uint16_t frame = 0;

  for (;;) {
    if (++frame == 0) /* 0 veut dire « pas encore d'image » */
      frame = 1;

    updateScene(&scene, comm[COMM_PAD], screenHeight);

    /* Publier la scène, puis lancer le Slave */
    *SHARED(&sharedScene) = scene;
    SHARED_BARRIER();
    comm[COMM_FRAME] = frame;

    drawScene(&scene, PLATFORM_CPU_MASTER, screenHeight);

    /* Attendre que le Slave ait fini ses lignes */
    while (comm[COMM_SLAVE_DONE] != frame)
      if (!platform_running())
        return;

    if (!flipFrameBuffer())
      return;
  }
}

/* ---- Slave -------------------------------------------------------------- */

void slave_main(void) {
  const int screenHeight = vdp_screen_height();
  uint16_t lastFrame = 0;

  for (;;) {
    uint16_t frame;
    while ((frame = comm[COMM_FRAME]) == lastFrame)
      if (!platform_running())
        return;

    SHARED_BARRIER();
    Scene scene = *SHARED(&sharedScene);

    drawScene(&scene, PLATFORM_CPU_SLAVE, screenHeight);

    SHARED_BARRIER();
    comm[COMM_SLAVE_DONE] = frame;
    lastFrame = frame;
  }
}

/* ---- Initialisation ----------------------------------------------------- */

static void initVDP(int screenHeight) {
  /* Mode Blank : l'échange de frame buffer est immédiat */
  *bitmapModeRegister = (*bitmapModeRegister & 0xFFFC);

  /* Une table de lignes dans chacun des deux frame buffers */
  fb_init_line_table(screenHeight);
  vdp_write_fbcr(vdp_read_fbcr() ^ FBCR_FS);
  fb_init_line_table(screenHeight);

  for (int i = 0; i < PALETTE_COLORS; i++)
    vdp_palette[i] = asset_palette[i];

  /* Mode Packed Pixel */
  *bitmapModeRegister = (*bitmapModeRegister & 0xFFFC) | 0x0001;
}

/* ---- Logique ------------------------------------------------------------ */

static int clamp(int v, int min, int max) {
  return v < min ? min : (v > max ? max : v);
}

static void updateScene(Scene *scene, unsigned pad, int screenHeight) {
  const int speed = (pad & PAD_C) ? SPEED_FAST : SPEED_SLOW;

  /* Direction de la croix : dx, dy dans -1, 0, 1 */
  const int dx = ((pad & PAD_RIGHT) ? 1 : 0) - ((pad & PAD_LEFT) ? 1 : 0);
  const int dy = ((pad & PAD_DOWN) ? 1 : 0) - ((pad & PAD_UP) ? 1 : 0);

  if (pad & PAD_START) {
    scene->sonicX = (int16_t)clamp(scene->sonicX + dx * speed, -200, 520);
    scene->sonicY = (int16_t)clamp(scene->sonicY + dy * speed, -200, 420);
    return;
  }

  /* Le contenu suit la flèche : il monte quand le défilement augmente */
  if (pad & PAD_A) {
    scene->scrollAX = (int16_t)(scene->scrollAX - dx * speed);
    scene->scrollAY = (int16_t)(scene->scrollAY - dy * speed);
  }
  if (pad & PAD_B) {
    scene->scrollBX = (int16_t)(scene->scrollBX - dx * speed);
    scene->scrollBY = (int16_t)(scene->scrollBY - dy * speed);
  }
  if (!(pad & (PAD_A | PAD_B)))
    scene->splitLine =
        (int16_t)clamp(scene->splitLine + dy * speed, 0, screenHeight);
}

/* ---- Dessin ------------------------------------------------------------- */

static void drawScene(const Scene *scene, int cpu, int screenHeight) {
  const Screen screen = {
      .height = screenHeight,
      .firstLine = cpu, /* Master : lignes paires, Slave : impaires */
      .lineStep = PLATFORM_CPU_COUNT,
  };

  /* Fond. La ligne y de l'écran montre la ligne (scrollY + y) du plan :
     le plan B ne « saute » pas quand la séparation bouge. */
  for (int y = screen.firstLine; y < screenHeight; y += screen.lineStep) {
    const uint32_t line = fb_offset(0, y);
    if (y < scene->splitLine)
      plane_draw_line(line, &asset_plane_a, &asset_tiles, scene->scrollAX,
                      scene->scrollAY + y);
    else
      plane_draw_line(line, &asset_plane_b, &asset_tiles, scene->scrollBX,
                      scene->scrollBY + y);
  }

  /* Sprites, du fond vers l'avant */
  sprite_draw(&screen, &asset_sonic, scene->sonicX, scene->sonicY);

  const int right = FB_WIDTH - asset_corner_tr.width;
  const int bottom = screenHeight - asset_corner_bl.height;
  sprite_draw(&screen, &asset_corner_tl, 0, 0);
  sprite_draw(&screen, &asset_corner_tr, right, 0);
  sprite_draw(&screen, &asset_corner_bl, 0, bottom);
  sprite_draw(&screen, &asset_corner_br, right, bottom);
}

/* Demande l'échange des frame buffers et attend qu'il soit fait.

   Sur 32X comme dans le simulateur, une demande faite pendant l'affichage
   prend effet au début du VBlank suivant, mais une demande faite PENDANT
   le VBlank prend effet aussitôt. Si une image est dessinée plus vite que
   le VBlank (c'est le cas sur l'hôte), plusieurs échanges auraient lieu
   dans le même VBlank : le jeu tournerait à plus de 60 images/s.
   On attend donc la fin du VBlank avant de demander l'échange : il a
   lieu au VBlank suivant, une fois par image au plus. */
static int flipFrameBuffer(void) {
  while (vdp_read_fbcr() & FBCR_VBLK)
    if (!platform_running())
      return 0;

  const uint16_t wanted = (vdp_read_fbcr() ^ FBCR_FS) & FBCR_FS;
  vdp_write_fbcr(wanted);

  while ((vdp_read_fbcr() & FBCR_FS) != wanted)
    if (!platform_running())
      return 0;
  return 1;
}
