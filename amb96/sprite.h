#ifndef SPRITE_H
#define SPRITE_H

#include "assets_types.h"
#include "framebuffer.h"

/* Dessine les lignes du sprite qui reviennent à ce CPU (voir Screen).
   (x, y) : coin haut-gauche, en pixels d'écran ; le sprite peut déborder,
   ou être entièrement hors de l'écran.

   L'index 0 est transparent, grâce à la zone overwrite du frame buffer
   (voir vdp.h) : le sprite y est copié tel quel, et le VDP ignore les
   octets nuls. Aucun test par pixel n'est fait ici. */
void sprite_draw(const Screen *screen, const Sprite *sprite, int x, int y);

#endif
