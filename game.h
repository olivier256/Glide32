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

/* Routine V_INT du 68000 (le programme Mega Drive de la cartouche).
   Le 68000 n'est pas émulé : la plate-forme appelle cette fonction au
   début de chaque VBlank, depuis la synchro vidéo. Elle doit être très
   courte (lire la manette, écrire les registres COMM) et ne toucher ni
   au frame buffer ni à la palette 32X, réservés aux SH2. */
void md_vint(void);

#endif
