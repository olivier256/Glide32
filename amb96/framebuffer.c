#include "framebuffer.h"

#include <stdint.h>

/* Seule écriture directe par vdp_dram, et non comptée : le simulateur
   relit la table de lignes comme des mots de l'hôte (voir bigendian.h),
   alors que vdp_dram_write16() range les octets dans l'ordre du SH2.
   Elle n'est faite qu'une fois, à l'initialisation. */
void fb_init_line_table(int screenHeight) {
  for (int y = 0; y < screenHeight; y++)
    vdp_dram[y] = (uint16_t)((FB_LINE_TABLE_BYTES + y * FB_WIDTH) / 2);
}

void fb_copy(uint32_t offset, const uint8_t *src, int n) {
  const uintptr_t mismatch = (uintptr_t)offset ^ (uintptr_t)src;

  if ((mismatch & 3) == 0) {
    /* Même alignement modulo 4 : octets jusqu'à une adresse multiple de 4,
       puis mots de 32 bits */
    for (; n > 0 && (offset & 3); n--)
      vdp_dram_write8(offset++, *src++);
    for (; n >= 4; n -= 4, offset += 4, src += 4)
      vdp_dram_write32(offset, ((uint32_t)src[0] << 24) |
                                   ((uint32_t)src[1] << 16) |
                                   ((uint32_t)src[2] << 8) | src[3]);
  } else if ((mismatch & 1) == 0) {
    /* Même parité : mots de 16 bits */
    if (n > 0 && (offset & 1)) {
      vdp_dram_write8(offset++, *src++);
      n--;
    }
    for (; n >= 2; n -= 2, offset += 2, src += 2)
      vdp_dram_write16(offset, (uint16_t)((src[0] << 8) | src[1]));
  }

  /* Le reste : tout si les alignements diffèrent, sinon la fin */
  for (; n > 0; n--)
    vdp_dram_write8(offset++, *src++);
}
