#ifndef PLANE_H
#define PLANE_H

#include <stdint.h>

#include "assets_types.h"

/* =========================================================================
   Plans de tiles.

   Un plan est une image de (widthTiles x TILE_SIZE) x (heightTiles x
   TILE_SIZE) pixels,
   qui boucle dans les deux directions. Le pixel (0, 0) de l'écran montre
   le pixel (scrollX, scrollY) du plan.
   ========================================================================= */

/* Dessine FB_WIDTH pixels à l'offset 'offset' du frame buffer de dessin
   (voir fb_offset) : la ligne worldY du plan, à partir de la colonne
   scrollX. worldY et scrollX peuvent être quelconques (négatifs, ou plus
   grands que le plan) : ils sont ramenés dans le plan. */
void plane_draw_line(uint32_t offset, const Tilemap *map,
                     const Tileset *tiles, int scrollX, int worldY);

#endif
