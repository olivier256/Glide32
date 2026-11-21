#include "plane.h"
#include "framebuffer.h"

/* La ligne est construite morceau par morceau : la fin de la première
   tile (à partir de la colonne scrollX), les tiles entières, puis le
   début de la dernière (avec des tiles de 8 pixels : 40 ou 41 morceaux
   par ligne). Les dimensions du plan étant des puissances de 2,
   le bouclage n'est qu'un masque.

   Chaque morceau est copié par fb_copy, comme par un memcpy ordinaire :
   par mots seulement si source et destination ont le même alignement.
   Avec un défilement horizontal impair, tout part octet par octet. */
void plane_draw_line(uint32_t offset, const Tilemap *map,
                     const Tileset *tiles, int scrollX, int worldY) {
  const int widthMask = (map->widthTiles << TILE_SHIFT) - 1;
  const int heightMask = (map->heightTiles << TILE_SHIFT) - 1;

  worldY &= heightMask;

  /* Rangée de tiles traversée par la ligne, et ligne dans chaque tile */
  const uint16_t *row = map->cells + (worldY >> TILE_SHIFT) * map->widthTiles;
  const uint8_t *tileLine =
      tiles->pixels + (worldY & (TILE_SIZE - 1)) * TILE_SIZE;

  int x = scrollX & widthMask;
  int remaining = FB_WIDTH;

  while (remaining > 0) {
    int xInTile = x & (TILE_SIZE - 1);
    int count = TILE_SIZE - xInTile;
    if (count > remaining)
      count = remaining;

    const uint8_t *src =
        tileLine + row[x >> TILE_SHIFT] * TILE_PIXELS + xInTile;
    fb_copy(offset, src, count);

    offset += (uint32_t)count;
    remaining -= count;
    x = (x + count) & widthMask;
  }
}
