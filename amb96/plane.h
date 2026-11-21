#ifndef PLANE_H
#define PLANE_H

#include <stdint.h>

#include "assets_types.h"

/* =========================================================================
   Plans de tiles.

   Un plan est une image de (widthTiles x 32) x (heightTiles x 32) pixels,
   qui boucle dans les deux directions. Le pixel (0, 0) de l'écran montre
   le pixel (scrollX, scrollY) du plan.
   ========================================================================= */

/* Dessine FB_WIDTH pixels : la ligne worldY du plan, à partir de la
   colonne scrollX. worldY et scrollX peuvent être quelconques (négatifs,
   ou plus grands que le plan) : ils sont ramenés dans le plan. */
void plane_draw_line(uint8_t *dst, const Tilemap *map, const Tileset *tiles,
                     int scrollX, int worldY);

#endif
