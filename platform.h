#ifndef PLATFORM_H
#define PLATFORM_H

/* =========================================================================
   Plate-forme hôte (macOS / MiniFB).
   Elle possède le main(), les threads et le temps : c'est elle qui cadence
   le balayage vidéo (262 lignes à 60 Hz) et qui lance les deux CPU,
   comme la BootROM.
   ========================================================================= */

/* ---- Pour le jeu ------------------------------------------------------- */

/* 0 quand l'hôte demande l'arrêt (fenêtre fermée). Une vraie console ne
   s'arrête jamais : c'est la seule fonction de la plate-forme que le jeu
   a le droit d'appeler, dans ses boucles d'attente. */
int platform_running(void);

/* Outil de mesure : temps écoulé depuis le début du dernier VBlank */
double platform_vblank_elapsed_ms(void);

/* ---- Pour les modules 32X ---------------------------------------------- */

/* Les deux SH2 sont identiques : c'est le câblage de leur broche MD5 qui
   fixe leur rôle au démarrage.
   - MD5 à la masse (0) : Master (IC1)
   - MD5 à Vcc (1)      : Slave  (IC2)
   La valeur de ces constantes reproduit le niveau de MD5. */
#define PLATFORM_CPU_MASTER 0 /* MD5 = 0 (masse) */
#define PLATFORM_CPU_SLAVE 1  /* MD5 = 1 (Vcc)   */
#define PLATFORM_CPU_COUNT 2

/* CPU émulé qui exécute l'appelant, ou -1 (thread de l'hôte) */
int platform_cpu_id(void);

/* Barrière mémoire entre threads (émule les accès cache-through du SH2) */
void platform_sync(void);

/* Interrompt le CPU indiqué (équivalent de ses lignes IRL) : la plate-forme
   exécute sa routine tant qu'une interruption y est active. */
void platform_raise_interrupt(int cpu);

#endif
