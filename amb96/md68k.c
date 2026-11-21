/* =========================================================================
   Programme 68000 (Mega Drive).

   Il ne fait plus qu'une chose : à chaque VBlank, lire la manette et la
   transmettre aux SH2 par un registre COMM. Toute la logique du jeu est
   sur le Master SH2.
   (Dans AMB96.asm, le 68000 calculait aussi les défilements.)

   Sur la console, ce serait du code 68000 : lecture du port $A10003
   (TH haut puis TH bas), puis écriture en $A15120 + 2 * COMM_PAD.
   ========================================================================= */

#include "game.h"
#include "interface.h"
#include "platform.h"
#include "shared.h"

void md_vint(void) { comm[COMM_PAD] = (uint16_t)platform_read_pad(); }
