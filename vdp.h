#ifndef VDP_H
#define VDP_H

#include <stdint.h>

/* Memory */
#define VDP_DRAM_SIZE (128 * 1024)
#define VDP_PALETTE_SIZE 256

extern uint16_t *vdp_dram;
extern uint16_t *displayFrameBuffer;
extern uint16_t vdp_dram0[VDP_DRAM_SIZE / 2];
extern uint16_t vdp_dram1[VDP_DRAM_SIZE / 2];
extern uint16_t vdp_palette[VDP_PALETTE_SIZE];

/* vdp.h / vdp.c */
extern volatile uint16_t *const bitmapModeRegister;
/* vdp.h / vdp.c */
extern volatile uint16_t *const frameBufferControlRegister;

#endif