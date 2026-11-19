#ifndef GAME_H
#define GAME_H

/* Points d'entrée des deux CPU, lancés par la plate-forme
   (comme la BootROM saute aux adresses de l'en-tête de la cartouche). */
void master_main(void);
void slave_main(void);

/* Routines d'interruption V_INT des deux CPU, appelées par la plate-forme
   (comme le SH2 saute à l'adresse lue dans sa table de vecteurs).
   Elles s'exécutent en interrompant le CPU, où qu'il en soit : elles
   doivent rester courtes, ne toucher que des variables volatile et
   n'appeler aucune fonction de la bibliothèque C (pas de printf, malloc...).
   Elles doivent acquitter l'interruption (interface_clear_vint), sinon
   elles sont rappelées indéfiniment. */
void master_vint(void);
void slave_vint(void);

#endif
