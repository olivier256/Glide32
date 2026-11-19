#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

/* =========================================================================
   Plate-forme hôte (macOS / MiniFB). Elle possède le main() et les threads.
   ========================================================================= */

int platform_init(int width, int height);
void platform_shutdown(void);

int platform_start_slave(void (*slave_main)(void)); /* lance le 2e CPU */
int platform_join_slave(void);                      /* attend sa fin */

/* Étape 1 seulement : encore appelée par le master, qui cadence les VBlank.
   Disparaîtra à l'étape 2 (synchro vidéo autonome). */
int platform_wait_vsync(void);

/* Barrière mémoire entre CPU émulés (utilisée par les modules 32X) */
void platform_sync(void);

/* Outil de mesure : temps écoulé depuis le début du dernier VBlank */
double platform_vblank_elapsed_ms(void);

#endif
