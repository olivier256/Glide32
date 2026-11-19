#ifndef GAME_H
#define GAME_H

/* Points d'entrée des deux CPU, lancés par la plate-forme
   (comme la BootROM saute aux adresses de l'en-tête de la cartouche). */
void master_main(void);
void slave_main(void);

#endif
