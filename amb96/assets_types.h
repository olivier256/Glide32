#ifndef ASSETS_TYPES_H
#define ASSETS_TYPES_H

#include <stdint.h>

/* =========================================================================
   Types des données graphiques, telles que les produit tools/bmp2c.

   Tous les pixels sont des index de palette sur 8 bits (mode Packed Pixel
   du VDP 32X). L'index 0 est transparent pour les sprites.
   ========================================================================= */

/* Les tiles sont carrées, de 32 x 32 pixels (comme dans AMB96 d'origine) */
#define TILE_SHIFT 5
#define TILE_SIZE (1 << TILE_SHIFT)
#define TILE_PIXELS (TILE_SIZE * TILE_SIZE)

/* Jeu de tiles partagé par les plans : 'count' tiles de TILE_PIXELS
   octets, rangées ligne par ligne. Au plus 256 tiles (index sur un octet). */
typedef struct {
  uint16_t count;
  const uint8_t *pixels;
} Tileset;

/* Plan : grille de widthTiles x heightTiles numéros de tile, ligne par
   ligne. Les deux dimensions sont des puissances de 2, pour que le
   défilement boucle avec un simple masque. */
typedef struct {
  uint16_t widthTiles;
  uint16_t heightTiles;
  const uint8_t *cells;
} Tilemap;

/* Sprite : width x height pixels, ligne par ligne */
typedef struct {
  uint16_t width;
  uint16_t height;
  const uint8_t *pixels;
} Sprite;

/* Palette : 256 couleurs au format du VDP 32X, 0bbbbbgggggrrrrr
   (le bit 15, priorité, est laissé à 0). */
#define PALETTE_COLORS 256

#endif
