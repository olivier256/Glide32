#include "vdp.h"

/* Memory */
uint16_t *vdp_dram = vdp_dram1;
uint16_t *displayFrameBuffer = vdp_dram0;
uint16_t vdp_dram0[VDP_DRAM_SIZE / 2];
uint16_t vdp_dram1[VDP_DRAM_SIZE / 2];
uint16_t vdp_palette[VDP_PALETTE_SIZE];

/* Registers */
static volatile uint16_t _bitmapModeRegister;
volatile uint16_t *const bitmapModeRegister = &_bitmapModeRegister;

static volatile uint16_t _frameBufferControlRegister;
volatile uint16_t *const frameBufferControlRegister =
    &_frameBufferControlRegister;