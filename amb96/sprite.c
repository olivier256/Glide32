#include "sprite.h"

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

  for (; row < rowMax; row += screen->lineStep) {
    const uint8_t *src = sprite->pixels + row * sprite->width + colMin;
    uint8_t *dst = screen->pixels + (y + row) * FB_WIDTH + x + colMin;

    for (int i = 0; i < width; i++) {
      uint8_t c = src[i];
      if (c != 0)
        dst[i] = c;
    }
  }
}
