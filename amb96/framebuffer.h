#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

#include "vdp.h"

/* =========================================================================
   Frame buffer du VDP 32X en mode Packed Pixel (un octet par pixel).

   La DRAM commence par la table de lignes : un mot par ligne affichée,
   qui donne l'adresse (en mots) des pixels de la ligne. On réserve
   256 mots (512 octets), comme AMB96 d'origine, puis les lignes se
   suivent, de FB_WIDTH octets chacune.
   ========================================================================= */

#define FB_WIDTH VDP_SCREEN_WIDTH
#define FB_LINE_TABLE_BYTES 512

/* Remplit la table de lignes du frame buffer de dessin */
void fb_init_line_table(int screenHeight);

/* Toutes les écritures du moteur dans le frame buffer passent par
   vdp_dram_write*() ou vdp_overwrite_write*(), pour être comptées par le
   simulateur (voir vdp.h). Seule exception : la table de lignes, à
   l'initialisation (voir framebuffer.c). */

/* Position du pixel (x, y) en octets depuis le début du frame buffer de
   dessin : c'est l'offset attendu par vdp_dram_write*() et
   vdp_overwrite_write*(). */
static inline uint32_t fb_offset(int x, int y) {
  return (uint32_t)(FB_LINE_TABLE_BYTES + y * FB_WIDTH + x);
}

/* Ce qu'un CPU dessine pendant une image : les lignes firstLine,
   firstLine + lineStep, firstLine + 2 * lineStep...
   Avec deux CPU : lineStep = 2, firstLine = 0 (Master) ou 1 (Slave).
   Chaque CPU possède entièrement ses lignes (fond puis sprites) : aucun
   des deux n'a besoin d'attendre l'autre au milieu de l'image. */
typedef struct {
  int height;
  int firstLine;
  int lineStep;
} Screen;

#endif
