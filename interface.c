#include "interface.h"
#include "platform.h"

/* Registres de communication */
volatile uint16_t comm[COMM_COUNT];

/* ---- Interruptions ------------------------------------------------------ */

/* Un masque et une V_INT en attente par CPU. La V_INT est mémorisée même
   si elle est masquée : la démasquer ensuite la déclenche aussitôt. Une
   routine qui veut l'éviter acquitte avant de démasquer. */
static volatile uint16_t _intMask[PLATFORM_CPU_COUNT];
static volatile int _vintPending[PLATFORM_CPU_COUNT];

static volatile unsigned long _raised = 0;
static volatile unsigned long _handled[PLATFORM_CPU_COUNT];
static volatile unsigned long _unacked[PLATFORM_CPU_COUNT];

uint16_t interface_read_intmask(void) {
  int cpu = platform_cpu_id();
  return cpu >= 0 ? _intMask[cpu] : 0;
}

void interface_write_intmask(uint16_t value) {
  int cpu = platform_cpu_id();
  if (cpu < 0)
    return;

  _intMask[cpu] = value & 0x000F;
  platform_sync();

  /* Effet immédiat : une V_INT déjà en attente se déclenche maintenant */
  if (interface_vint_active(cpu))
    platform_raise_interrupt(cpu);
}

void interface_clear_vint(void) {
  int cpu = platform_cpu_id();
  if (cpu < 0)
    return;

  _vintPending[cpu] = 0;
  platform_sync();
}

void interface_raise_vint(void) {
  _raised++;

  for (int cpu = 0; cpu < PLATFORM_CPU_COUNT; cpu++) {
    _vintPending[cpu] = 1;
    platform_sync();

    if (_intMask[cpu] & INTMASK_V)
      platform_raise_interrupt(cpu);
  }
}

int interface_vint_active(int cpu) {
  return _vintPending[cpu] && (_intMask[cpu] & INTMASK_V);
}

void interface_count_handled(int cpu, int acknowledged) {
  _handled[cpu]++;
  if (!acknowledged)
    _unacked[cpu]++;
}

void interface_get_stats(interface_stats *stats) {
  stats->raised = _raised;
  for (int i = 0; i < PLATFORM_CPU_COUNT; i++) {
    stats->handled[i] = _handled[i];
    stats->unacked[i] = _unacked[i];
  }
}
