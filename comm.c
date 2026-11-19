#include "comm.h"
#include "platform.h"

static volatile uint16_t comm_regs[COMM_COUNT];

/* platform_sync() émule la garantie qu'offre la 32X en accès cache-through :
   les écritures d'un SH2 sont vues par l'autre dans l'ordre du programme. */

void comm_write(int reg, uint16_t value) {
  platform_sync(); /* les écritures précédentes doivent être visibles */
  comm_regs[reg] = value;
}

uint16_t comm_read(int reg) {
  uint16_t value = comm_regs[reg];
  platform_sync(); /* rien ne doit être lu/écrit avant ce registre */
  return value;
}
