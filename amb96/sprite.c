#include "sprite.h"
#include "vdp.h"

/* Avec AMB96_ASM, sprite_copy_row est fournie par l'assembleur
   (amb96/sh2/sprite_copy_row.S) : la version C reste la référence. */
#ifndef AMB96_ASM

/* Écritures par mots de 16 bits (deux pixels), qui doivent tomber sur une
   adresse paire : un premier pixel seul si la destination est impaire,
   puis des paires, puis un dernier pixel seul s'il en reste un.
   La source n'a pas besoin d'être alignée : les mots sont reconstitués
   octet par octet.

   Un mot dont les deux pixels sont transparents n'est pas écrit : le VDP
   l'aurait ignoré (zone overwrite), mais l'accès au bus aurait eu lieu.
   Même chose pour un pixel seul transparent. */
void sprite_copy_row(uint32_t offset, const uint8_t *src, int n) {
  if (offset & 1) {
    if (*src != 0)
      vdp_overwrite_write8(offset, *src);
    offset++;
    src++;
    n--;
  }

  for (; n >= 2; n -= 2) {
    const uint16_t pair = (uint16_t)((src[0] << 8) | src[1]);
    if (pair != 0)
      vdp_overwrite_write16(offset, pair);
    offset += 2;
    src += 2;
  }

  if (n > 0 && *src != 0)
    vdp_overwrite_write8(offset, *src);
}

#endif /* AMB96_ASM */

void sprite_draw(const Screen *screen, const Sprite *sprite, int x, int y) {
  /* Partie visible du sprite, en coordonnées du sprite */
  int colMin = x < 0 ? -x : 0;
  int colMax = sprite->width;
  if (x + colMax > FB_WIDTH)
    colMax = FB_WIDTH - x;

  int rowMin = y < 0 ? -y : 0;
  int rowMax = sprite->height;
  if (y + rowMax > screen->height)
    rowMax = screen->height - y;

  if (colMin >= colMax || rowMin >= rowMax)
    return;

  /* Première ligne visible qui revient à ce CPU */
  int row = rowMin;
  int phase = (y + row - screen->firstLine) % screen->lineStep;
  if (phase < 0)
    phase += screen->lineStep;
  if (phase != 0)
    row += screen->lineStep - phase;

  const int width = colMax - colMin;

  for (; row < rowMax; row += screen->lineStep)
    sprite_copy_row(fb_offset(x + colMin, y + row),
            sprite->pixels + row * sprite->width + colMin, width);
}
