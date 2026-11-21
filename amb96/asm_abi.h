#ifndef ASM_ABI_H
#define ASM_ABI_H

/* =========================================================================
   Ce que les routines assembleur SH2 (dossier amb96/sh2) doivent savoir des
   données C. Fichier lu à la fois par le C et par l'assembleur : que des
   #define.

   Côté C, plane.c vérifie à la compilation (pour SH2 seulement) que ces
   positions correspondent bien aux structures de assets_types.h. Si une
   structure change, la compilation SH2 s'arrête au lieu de produire un
   binaire faux.
   ========================================================================= */

/* Tilemap : widthTiles (16 bits), heightTiles (16 bits), cells (pointeur) */
#define TILEMAP_WIDTH_TILES 0
#define TILEMAP_HEIGHT_TILES 2
#define TILEMAP_CELLS 4

/* Tileset : count (16 bits), [2 octets de bourrage], pixels (pointeur) */
#define TILESET_PIXELS 4

/* Frame buffer du VDP, zone normale, adresse sans cache */
#define FB_DRAM_ADDRESS 0x24000000

/* Même frame buffer, zone overwrite (octets nuls ignorés), sans cache */
#define FB_OVERWRITE_ADDRESS 0x24020000

/* Pixels par ligne d'écran (= FB_WIDTH, vérifié par plane.c) */
#define FB_LINE_PIXELS 320

#endif
