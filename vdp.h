#ifndef VDP_H
#define VDP_H

#include <stdint.h>

/* =========================================================================
   VDP 32X (émulé) : module sans thread.
   ========================================================================= */

#define VDP_SCREEN_WIDTH 320
#define VDP_DRAM_SIZE (128 * 1024)
#define VDP_PALETTE_SIZE 256

/* Frame Buffer Control Register */
#define FBCR_VBLK 0x8000
#define FBCR_HBLK 0x4000
#define FBCR_FS 0x0001

/* ---- Vu des CPU (jeu) -------------------------------------------------- */

/* Frame buffer de dessin (celui qui n'est pas affiché) */
extern uint16_t *vdp_dram;
extern uint16_t vdp_palette[VDP_PALETTE_SIZE];

/* Registres */
extern volatile uint16_t *const bitmapModeRegister;
uint16_t vdp_read_fbcr(void);
void vdp_write_fbcr(uint16_t value);

/* Nombre de lignes affichées */
int vdp_screen_height(void);

/* ---- Vu de la plate-forme ---------------------------------------------- */

void vdp_init(int screenHeight);

/* Appelées par la synchro vidéo aux frontières de lignes */
void vdp_begin_vblank(void);
void vdp_end_vblank(void);

/* Décode une ligne affichée du frame buffer affiché vers RGB
   (VDP_SCREEN_WIDTH pixels, 0x00RRGGBB). */
void vdp_render_line(int line, uint32_t *rgb);

/* Diagnostics : nombre de VBlank, et nombre de VBlank vus par chaque CPU
   (au moins une lecture du FBCR avec VBLK = 1 pendant ce VBlank). */
typedef struct {
  unsigned long vblanks;
  unsigned long seen[2];
} vdp_stats;

void vdp_get_stats(vdp_stats *stats);

#endif
