#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdint.h>

/* =========================================================================
   Interface 32X (émulée) : IC4, circuit 315-5818, « MEGA Drive I/F ».
   Module sans thread.

   Sur la vraie machine, l'IC4 regroupe les registres système
   (2000 4000h - 2000 403Fh côté SH2) : masques et acquittement des
   interruptions, compteur H, FIFO DREQ, PWM, registres de communication...
   Pour l'instant, seuls les registres de communication sont émulés.
   ========================================================================= */

/* ---- Registres de communication ------------------------------------------
   8 mots partagés par les CPU (2000 4020h - 2000 402Eh).
   Rangés comme sur le SH2 : accès 16 bits par read16/write16 (sh2mem.h),
   accès 8 bits directs par un cast en uint8_t (2000 4020h = octet de poids
   fort de COMM0, comme sur 32X).
   volatile : chaque lecture relit vraiment le registre (boucles d'attente). */

#define COMM_COUNT 8

extern volatile uint16_t comm[COMM_COUNT];

#endif
