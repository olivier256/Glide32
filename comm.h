#ifndef COMM_H
#define COMM_H

#include <stdint.h>

/* =========================================================================
   Registres de communication 32X (émulés) : module sans thread.
   Le manuel en décrit 8 (2000 4020h - 2000 402Eh).
   ========================================================================= */

#define COMM_COUNT 8

void comm_write(int reg, uint16_t value);
uint16_t comm_read(int reg);

#endif
