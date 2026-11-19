#ifndef SH2MEM_H
#define SH2MEM_H

#include <stdint.h>

/* =========================================================================
   Accès mémoire vus du SH2.

   Convention : toute la mémoire de la 32X (frame buffer, registres de
   communication, données brutes en SDRAM...) est rangée comme sur le SH2,
   en gros-boutiste : l'octet de poids fort est à l'adresse la plus basse.

   - Accès 8 bits : directs, par un pointeur uint8_t, sans précaution.
   - Accès 16 et 32 bits : TOUJOURS par read16/write16/read32/write32.
     Sur 32X, ce sont de simples MOV.W / MOV.L ; sur un hôte
     petit-boutiste (Mac), elles échangent les octets.

   Les variables C ordinaires (int, structures) n'en ont pas besoin : les
   deux CPU émulés sont compilés pour le même hôte. Seules les zones
   mémoire lues avec une autre taille que celle de l'écriture, ou lues par
   le matériel (VDP), sont concernées.

   Adresses alignées obligatoires : 2 octets pour 16 bits, 4 pour 32 bits,
   comme sur le SH2.
   ========================================================================= */

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

static inline uint16_t read16(const volatile void *address) {
  uint16_t v = *(const volatile uint16_t *)address;
  return (uint16_t)((v << 8) | (v >> 8));
}

static inline void write16(volatile void *address, uint16_t value) {
  *(volatile uint16_t *)address = (uint16_t)((value << 8) | (value >> 8));
}

static inline uint32_t read32(const volatile void *address) {
  uint32_t v = *(const volatile uint32_t *)address;
  return (v << 24) | ((v << 8) & 0x00FF0000u) | ((v >> 8) & 0x0000FF00u) |
         (v >> 24);
}

static inline void write32(volatile void *address, uint32_t value) {
  *(volatile uint32_t *)address =
      (value << 24) | ((value << 8) & 0x00FF0000u) |
      ((value >> 8) & 0x0000FF00u) | (value >> 24);
}

#else /* SH2, ou hôte gros-boutiste : accès direct */

static inline uint16_t read16(const volatile void *address) {
  return *(const volatile uint16_t *)address;
}

static inline void write16(volatile void *address, uint16_t value) {
  *(volatile uint16_t *)address = value;
}

static inline uint32_t read32(const volatile void *address) {
  return *(const volatile uint32_t *)address;
}

static inline void write32(volatile void *address, uint32_t value) {
  *(volatile uint32_t *)address = value;
}

#endif

#endif
