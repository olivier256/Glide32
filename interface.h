#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdint.h>

/* =========================================================================
   Interface 32X (émulée) : IC4, circuit 315-5818, « MEGA Drive I/F ».
   Module sans thread.

   Sur la vraie machine, l'IC4 regroupe les registres système
   (2000 4000h - 2000 403Fh côté SH2) : masques et acquittement des
   interruptions, compteur H, FIFO DREQ, PWM, registres de communication...
   Émulés pour l'instant : les registres de communication, et V_INT.
   ========================================================================= */

/* ---- Registres de communication ------------------------------------------
   8 mots partagés par les CPU (2000 4020h - 2000 402Eh). Accès direct,
   comme vdp_palette.
   volatile : chaque lecture relit vraiment le registre (boucles d'attente).
   Attention à l'accès par octet via un cast : l'hôte est petit-boutiste,
   le SH2 gros-boutiste (sur 32X, 2000 4020h est l'octet de poids FORT). */

#define COMM_COUNT 8

extern volatile uint16_t comm[COMM_COUNT];

/* ---- Interruptions : vu des CPU (jeu) -------------------------------------
   Interrupt Mask Register (2000 4000h, bits 3 à 0). Il en existe un par
   CPU, à la même adresse : chaque fonction agit sur le registre du CPU
   qui l'appelle. Bit à 1 = interruption transmise au CPU ; à 0 = masquée
   (valeur au démarrage).

   Le masque et l'acquittement passent par des fonctions plutôt que par un
   pointeur, pour la même raison que le FBCR (vdp_write_fbcr) : l'écriture
   a un effet immédiat. Si l'on démasque V_INT alors qu'elle est déjà en
   attente, elle doit se déclencher aussitôt ; avec une simple écriture en
   mémoire, personne ne le saurait. */

#define INTMASK_PWM 0x0001
#define INTMASK_CMD 0x0002
#define INTMASK_H 0x0004
#define INTMASK_V 0x0008 /* seule source émulée pour l'instant */

uint16_t interface_read_intmask(void);
void interface_write_intmask(uint16_t value);

/* V Interrupt Clear Register (2000 4016h) : acquitte V_INT pour le CPU
   appelant. Une V_INT reste active tant qu'elle n'est pas acquittée : la
   routine d'interruption doit l'acquitter, sinon elle est rappelée dès
   son retour, indéfiniment (comme sur 32X). */
void interface_clear_vint(void);

/* ---- Interruptions : vu de la plate-forme ---------------------------------
 */

/* Le VDP signale le début du VBlank. V_INT est mémorisée pour chaque CPU
   et transmise à ceux qui l'ont démasquée. */
void interface_raise_vint(void);

/* 1 si une V_INT est en attente et démasquée pour ce CPU */
int interface_vint_active(int cpu);

/* Diagnostics */
typedef struct {
  unsigned long raised;     /* V_INT levées par le VDP */
  unsigned long handled[2]; /* routines exécutées, par CPU */
  unsigned long unacked[2]; /* routines revenues sans acquitter */
} interface_stats;

void interface_count_handled(int cpu, int acknowledged);
void interface_get_stats(interface_stats *stats);

#endif
