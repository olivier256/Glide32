#ifndef SPRITE_H
#define SPRITE_H

#include "assets_types.h"
#include "framebuffer.h"

/* Dessine les lignes du sprite qui reviennent à ce CPU (voir Screen).
   (x, y) : coin haut-gauche, en pixels d'écran ; le sprite peut déborder,
   ou être entièrement hors de l'écran. L'index 0 est transparent.

   Sur 32X, la transparence peut être faite par le matériel : écrit dans
   la zone « overwrite » du frame buffer (DRAM + 20000h), un octet nul est
   ignoré. Le simulateur ne l'émule pas encore : le test est fait ici. */
void sprite_draw(const Screen *screen, const Sprite *sprite, int x, int y);

#endif
