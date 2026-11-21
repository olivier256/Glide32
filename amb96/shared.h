#ifndef SHARED_H
#define SHARED_H

#include <stdint.h>

/* =========================================================================
   Ce que se partagent les trois processeurs.

   1. Registres COMM (interface.h) : 8 mots visibles du 68000 et des deux
      SH2, sans cache. Ils servent de boîtes aux lettres et de signaux.
   2. La scène de l'image en cours (Scene) : écrite par le Master, lue par
      le Slave, en mémoire partagée (SDRAM).
   ========================================================================= */

/* ---- Registres COMM ----------------------------------------------------- */

#define COMM_PAD 0        /* 68000 -> SH2 : boutons de la manette (PAD_*) */
#define COMM_FRAME 1      /* Master -> Slave : image à dessiner (1, 2, ...) */
#define COMM_SLAVE_DONE 2 /* Slave -> Master : dernière image terminée */
/* COMM 3 à 7 : libres */

/* ---- Scène ---------------------------------------------------------------
   Tout ce qu'il faut pour dessiner une image. Le Master la calcule au début
   de l'image, la publie, puis les deux CPU dessinent la même. */

typedef struct {
  int16_t splitLine;         /* première ligne du plan B */
  int16_t scrollAX, scrollAY; /* défilement du plan A (haut) */
  int16_t scrollBX, scrollBY; /* défilement du plan B (bas) */
  int16_t sonicX, sonicY;    /* position du sprite, en pixels d'écran */
} Scene;

/* ---- Mémoire partagée entre les SH2 --------------------------------------
   Chaque SH2 a son propre cache : sans précaution, le Slave peut lire une
   copie périmée de ce que le Master vient d'écrire.
   - Sur 32X : on accède à la donnée par son adresse « cache-through »
     (adresse | 2000 0000h), qui contourne le cache.
   - Sur le simulateur : les threads de l'hôte n'ont pas ce problème de
     cache, mais le processeur (Apple Silicon) peut réordonner les
     écritures : il faut une barrière mémoire.
   SHARED(p) donne le pointeur à utiliser ; SHARED_BARRIER() sépare
   l'écriture des données de celle du signal COMM qui les annonce (et,
   côté lecteur, la lecture du signal de celle des données). */

#if defined(__sh__)
#define SHARED(p) ((__typeof__(p))((uintptr_t)(p) | 0x20000000u))
#define SHARED_BARRIER() __asm__ volatile("" ::: "memory")
#else
#define SHARED(p) (p)
#define SHARED_BARRIER() __sync_synchronize()
#endif

#endif
