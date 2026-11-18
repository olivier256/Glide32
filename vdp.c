#include "vdp.h"

/* Memory */
uint16_t vdp_dram0[VDP_DRAM_SIZE / 2];
uint16_t vdp_dram1[VDP_DRAM_SIZE / 2];
uint16_t vdp_palette[VDP_PALETTE_SIZE];

uint16_t *vdp_dram = vdp_dram1;
uint16_t *displayFrameBuffer = vdp_dram0;

/* Registers */
static volatile uint16_t _bitmapModeRegister;
volatile uint16_t *const bitmapModeRegister = &_bitmapModeRegister;

/* représente la valeur effectivement appliquée */
static uint16_t _activeFS = 0;
/* conserve la dernière valeur écrite */
static uint16_t _requestedFS = 0;
static int _inVBlank = 0;

/* met à jour les deux pointeurs de mémoire */
static void applyFrameBuffer(void) {
  _activeFS = _requestedFS;

  displayFrameBuffer = _activeFS ? vdp_dram1 : vdp_dram0;
  vdp_dram = _activeFS ? vdp_dram0 : vdp_dram1;
}

/* renvoie la valeur active, pas nécessairement la valeur positionnée */
uint16_t vdp_read_fbcr(void) {
  uint16_t value = _activeFS ? FBCR_FS : 0;

  if (_inVBlank)
    value |= FBCR_VBLK;

  return value;
}

void vdp_write_fbcr(uint16_t value) {
  _requestedFS = value & FBCR_FS;

  if (_inVBlank || ((*bitmapModeRegister & 3) == 0))
    applyFrameBuffer();
}

void vdp_begin_vblank(void) {
  _inVBlank = 1;

  applyFrameBuffer();
}

void vdp_end_vblank(void) { _inVBlank = 0; }
