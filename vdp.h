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

/* Registers */
extern volatile uint16_t *const bitmapModeRegister;

/* Frame Buffer Control Register */
#define FBCR_VBLK 0x8000
#define FBCR_HBLK 0x4000
#define FBCR_FS 0x0001

uint16_t vdp_read_fbcr(void);
void vdp_write_fbcr(uint16_t value);

void vdp_begin_vblank(void);
void vdp_end_vblank(void);

#endif