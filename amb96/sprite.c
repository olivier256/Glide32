#include "sprite.h"
#include "vdp.h"

/* Copie n pixels dans la zone overwrite, à partir de l'offset 'offset'.
   Écritures par mots de 16 bits (deux pixels), qui doivent tomber sur une
   adresse paire : un premier pixel seul si la destination est impaire,
   puis des paires, puis un dernier pixel seul s'il en reste un.
   La source n'a pas besoin d'être alignée : les mots sont reconstitués
   octet par octet.

   Amélioration possible sur SH2 : des écritures 32 bits (quatre pixels)
   une fois la destination alignée sur 4. */
static void copyRow(uint32_t offset, const uint8_t *src, int n) {
  if (offset & 1) {
    vdp_overwrite_write8(offset, *src);
    offset++;
    src++;
    n--;
  }

  for (; n >= 2; n -= 2) {
    vdp_overwrite_write16(offset, (uint16_t)((src[0] << 8) | src[1]));
    offset += 2;
    src += 2;
  }

  if (n > 0)
    vdp_overwrite_write8(offset, *src);
}

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
    copyRow(fb_offset(x + colMin, y + row),
            sprite->pixels + row * sprite->width + colMin, width);
}
