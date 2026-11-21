#ifndef ASSETS_TYPES_H
#define ASSETS_TYPES_H

#include <stdint.h>

/* =========================================================================
   Types des données graphiques, telles que les produit tools/bmp2c.

   Tous les pixels sont des index de palette sur 8 bits (mode Packed Pixel
   du VDP 32X). L'index 0 est transparent pour les sprites.
   ========================================================================= */

/* Les tiles sont carrées, de 8 x 8 pixels (comme sur le VDP Mega Drive).
   C'est la seule constante à changer pour une autre taille (puissance de
   2) : bmp2c et le moteur la lisent tous deux ici. */
#define TILE_SHIFT 3
#define TILE_SIZE (1 << TILE_SHIFT)
#define TILE_PIXELS (TILE_SIZE * TILE_SIZE)

/* Jeu de tiles partagé par les plans : 'count' tiles de TILE_PIXELS
   octets, rangées ligne par ligne. */
#define TILESET_MAX_TILES 65535

typedef struct {
  uint16_t count;
  const uint8_t *pixels;
} Tileset;

/* Plan : grille de widthTiles x heightTiles numéros de tile (16 bits),
   ligne par ligne. Les deux dimensions sont des puissances de 2, pour que
   le défilement boucle avec un simple masque. */
typedef struct {
  uint16_t widthTiles;
  uint16_t heightTiles;
  const uint16_t *cells;
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
