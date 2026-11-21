#include "plane.h"
#include "asm_abi.h"
#include "framebuffer.h"
#include "vdp.h"

#include <stddef.h>

/* ---- Accord avec la version assembleur (amb96/sh2/plane_draw_line.S) ----
   asm_abi.h décrit pour l'assembleur la position des champs des
   structures. Si elles changent, ces vérifications arrêtent la compilation.
   Les positions ne valent que pour SH2 (pointeurs de 32 bits). */
#define STATIC_CHECK(name, cond) typedef char name[(cond) ? 1 : -1]

STATIC_CHECK(check_line_pixels, FB_LINE_PIXELS == FB_WIDTH);
#if defined(__sh__)
STATIC_CHECK(check_width, offsetof(Tilemap, widthTiles) == TILEMAP_WIDTH_TILES);
STATIC_CHECK(check_height,
             offsetof(Tilemap, heightTiles) == TILEMAP_HEIGHT_TILES);
STATIC_CHECK(check_cells, offsetof(Tilemap, cells) == TILEMAP_CELLS);
STATIC_CHECK(check_pixels, offsetof(Tileset, pixels) == TILESET_PIXELS);
#endif

/* Avec AMB96_ASM, plane_draw_line est fournie par l'assembleur : la
   version C ci-dessous reste la référence, mais n'est pas compilée. */
#ifndef AMB96_ASM

/* =========================================================================
   Dessin d'une ligne de plan.

   Règle : dans le frame buffer, uniquement des écritures 16 bits, à des
   adresses paires. Une ligne d'écran commence toujours à une adresse
   paire et on l'écrit à partir de x = 0 : la destination est toujours
   paire. C'est la source qui hérite de la parité du défilement X.

   - Défilement pair : la source est paire, elle aussi. Chaque mot écrit
     est lu d'un coup (MOV.W) dans la tile.
   - Défilement impair : la source est impaire. On lit les deux pixels
     séparément (deux MOV.B, depuis le cache) et on les assemble en un mot
     avant de l'écrire (MOV.W). Le coût supplémentaire est pour le CPU,
     pas pour le bus du VDP.

   La parité ne change pas en cours de ligne (tiles et plan ont une
   largeur paire) : le choix de la boucle se fait une fois par ligne.

   La ligne est parcourue tile par tile : la fin de la première tile (à
   partir de la colonne scrollX), les tiles entières, puis le début de la
   dernière (20 ou 21 morceaux avec des tiles de 16 pixels). Les dimensions
   du plan étant des puissances de 2, le bouclage n'est qu'un masque.

   Version finale prévue : lectures 32 bits alignées (MOV.L) et décalages
   en registres, XTRCT pour le cas pair non multiple de 4.
   ========================================================================= */

/* Deux pixels, l'un derrière l'autre, lus à une adresse paire : un MOV.W
   sur SH2. Sur l'hôte (petit-boutiste), on rétablit l'ordre du SH2 :
   pixel de gauche dans l'octet de poids fort. */
static inline uint16_t readPair(const uint8_t *p) {
#if defined(__sh__)
  return *(const uint16_t *)p;
#else
  return (uint16_t)((p[0] << 8) | p[1]);
#endif
}

/* Assemble deux pixels lus séparément (deux MOV.B) en un mot */
static inline uint16_t makePair(uint8_t left, uint8_t right) {
  return (uint16_t)((left << 8) | right);
}

/* Ce qu'il faut pour suivre la ligne du plan, tile par tile */
typedef struct {
  const uint16_t *row;     /* numéros de tile de la rangée traversée */
  const uint8_t *tileLine; /* ligne voulue dans la tile 0 */
  int widthMask;           /* largeur du plan en pixels, moins 1 */
} PlaneRow;

/* Premier pixel du morceau qui commence à la colonne x du plan, et
   nombre de pixels jusqu'à la fin de la tile (au plus 'remaining') */
static inline const uint8_t *segmentAt(const PlaneRow *pr, int x,
                                       int remaining, int *count) {
  const int xInTile = x & (TILE_SIZE - 1);
  int n = TILE_SIZE - xInTile;
  *count = n < remaining ? n : remaining;
  return pr->tileLine + pr->row[x >> TILE_SHIFT] * TILE_PIXELS + xInTile;
}

/* Défilement pair : chaque morceau a une longueur paire et commence à une
   adresse paire, dans la tile comme dans le frame buffer. */
static void drawLineEven(uint32_t offset, const PlaneRow *pr, int x) {
  int remaining = FB_WIDTH;

  while (remaining > 0) {
    int count;
    const uint8_t *src = segmentAt(pr, x, remaining, &count);

    for (int i = 0; i < count; i += 2) {
      vdp_dram_write16(offset, readPair(src + i));
      offset += 2;
    }

    remaining -= count;
    x = (x + count) & pr->widthMask;
  }
}

/* Défilement impair : chaque mot écrit réunit un pixel de colonne impaire
   et le suivant. Le dernier pixel d'une tile (colonne 15) va donc avec le
   premier de la tile suivante : il est gardé de côté ('carry') en
   attendant le morceau suivant. */
static void drawLineOdd(uint32_t offset, const PlaneRow *pr, int x) {
  int remaining = FB_WIDTH;
  uint8_t carry = 0;
  int hasCarry = 0;

  while (remaining > 0) {
    int count;
    const uint8_t *src = segmentAt(pr, x, remaining, &count);
    int i = 0;

    if (hasCarry) {
      vdp_dram_write16(offset, makePair(carry, src[0]));
      offset += 2;
      i = 1;
      hasCarry = 0;
    }

    for (; i + 1 < count; i += 2) {
      vdp_dram_write16(offset, makePair(src[i], src[i + 1]));
      offset += 2;
    }

    if (i < count) {
      carry = src[i];
      hasCarry = 1;
    }

    remaining -= count;
    x = (x + count) & pr->widthMask;
  }
  /* FB_WIDTH étant pair, il ne reste jamais de pixel en attente ici */
}

void plane_draw_line(uint32_t offset, const Tilemap *map,
                     const Tileset *tiles, int scrollX, int worldY) {
  const int heightMask = (map->heightTiles << TILE_SHIFT) - 1;
  worldY &= heightMask;

  const PlaneRow pr = {
      .row = map->cells + (worldY >> TILE_SHIFT) * map->widthTiles,
      .tileLine = tiles->pixels + (worldY & (TILE_SIZE - 1)) * TILE_SIZE,
      .widthMask = (map->widthTiles << TILE_SHIFT) - 1,
  };

  const int x = scrollX & pr.widthMask;
  if (x & 1)
    drawLineOdd(offset, &pr, x);
  else
    drawLineEven(offset, &pr, x);
}

#endif /* AMB96_ASM */
