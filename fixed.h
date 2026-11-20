#ifndef FIXED_H
#define FIXED_H

#include <stdint.h>

/* =========================================================================
   Virgule fixe 16.16, commune au jeu, à tnl et à Glide.
   Le SH2 n'a pas de FPU : aucun float dans le code du jeu.
   ========================================================================= */

typedef int32_t Fixed;

#define FIXED_ONE ((Fixed)0x00010000)
#define FIXED_HALF ((Fixed)0x00008000)
#define FIXED(i) ((Fixed)((i) * FIXED_ONE))

/* a * b. Sur SH2 : DMULS.L (32 x 32 -> 64), puis XTRCT. */
static inline Fixed fixed_mul(Fixed a, Fixed b) {
  return (Fixed)(((int64_t)a * b) >> 16);
}

/* a / b (b != 0). Sur SH2 : le diviseur matériel (64 / 32 -> 32). */
static inline Fixed fixed_div(Fixed a, Fixed b) {
  return (Fixed)(((int64_t)a * FIXED_ONE) / b);
}

#endif
