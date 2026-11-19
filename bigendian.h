#ifndef BIGENDIAN_H
#define BIGENDIAN_H

#include <stdint.h>

/* =========================================================================
   Valeurs 16 et 32 bits dans l'ordre du SH2 (gros-boutiste).

   BE16(v) / BE32(v) renvoient v telle qu'il faut l'écrire en mémoire (ou
   telle qu'on l'a relue) pour que ses octets se suivent dans l'ordre du
   SH2 : octet de poids fort à l'adresse la plus basse.
   - Sur 32X (ou tout hôte gros-boutiste) : ne font rien.
   - Sur un hôte petit-boutiste (Mac) : échangent les octets.
   L'échange appliqué deux fois redonne la valeur : la même macro sert à
   l'écriture et à la relecture.

   À utiliser pour les zones que le matériel lit octet par octet, en
   particulier les pixels du mode Packed Pixel écrits par mot :
       vdp_dram[lineAddr + x / 2] = BE16(clcr);     (2 pixels, = MOV.W)
       *(uint32_t *)p = BE32(p0p1p2p3);             (4 pixels, = MOV.L)
   Pas pour la table de lignes, que le VDP lit comme un mot de l'hôte.
   ========================================================================= */

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define BE16(v) ((uint16_t)__builtin_bswap16((uint16_t)(v)))
#define BE32(v) ((uint32_t)__builtin_bswap32((uint32_t)(v)))
#else
#define BE16(v) ((uint16_t)(v))
#define BE32(v) ((uint32_t)(v))
#endif

#endif
