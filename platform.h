#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

int platform_init(int width, int height);
int platform_wait_vsync(void);
void platform_shutdown(void);

int platform_start_slave(void (*slave_main)(void)); /* lance le 2e CPU */
int platform_join_slave(void);                      /* attend sa fin */

double platform_vblank_elapsed_ms(void);

/* Garantit que les ecritures precedentes sont visibles par l'autre CPU.
   Sur 32X : rien a faire si on passe par les adresses cache-through. */
void platform_sync(void);
#endif