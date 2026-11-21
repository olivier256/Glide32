#ifndef SPRITE_H
#define SPRITE_H

#include "assets_types.h"
#include "framebuffer.h"

/* Dessine les lignes du sprite qui reviennent à ce CPU (voir Screen).
   (x, y) : coin haut-gauche, en pixels d'écran ; le sprite peut déborder,
   ou être entièrement hors de l'écran.

   L'index 0 est transparent, grâce à la zone overwrite du frame buffer
   (voir vdp.h) : le VDP ignore les octets nuls. Les mots entièrement
   transparents ne sont même pas écrits (voir sprite_copy_row). */
void sprite_draw(const Screen *screen, const Sprite *sprite, int x, int y);

/* Copie n pixels d'une ligne de sprite (src) dans la zone overwrite, à
   partir de l'offset 'offset' du frame buffer de dessin, en sautant les
   mots entièrement transparents. Utilisée par sprite_draw ; remplaçable
   par sa version assembleur (amb96/sh2/sprite_copy_row.S, option
   AMB96_ASM). */
void sprite_copy_row(uint32_t offset, const uint8_t *src, int n);

#endif
