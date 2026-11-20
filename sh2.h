#ifndef SH2_H
#define SH2_H

/* =========================================================================
   SH2 (IC1, IC2) : ce que le processeur lui-même offre au code du jeu.
   Module sans thread.
   ========================================================================= */

/* Rend cohérente la mémoire partagée (SDRAM) entre les deux CPU :
   - les écritures faites par ce CPU sont visibles de l'autre avant toute
     écriture qui suit (typiquement : un registre COMM qui les annonce) ;
   - les lectures qui suivent voient les écritures de l'autre CPU, si
     elles ont été annoncées avant.

   Sur 32X : le cache du SH2 est à écriture traversante et le bus ne
   réordonne pas les écritures, le premier point est donc acquis. Le second
   correspond à la purge du cache (registre CCR), ou à la lecture des
   données partagées par leur adresse cache-through (2xxx xxxxh).
   Sur l'hôte : une barrière mémoire complète. */
void sh2_sync(void);

#endif
