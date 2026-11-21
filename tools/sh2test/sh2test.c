/* =========================================================================
   Banc d'essai des routines assembleur SH2, sous qemu-sh4eb (SH gros-
   boutiste, mode utilisateur Linux). Compile la version C de
   plane_draw_line (renommée plane_draw_line_c) et la version assembleur
   dans le même programme, pour SH2, et compare leurs lignes pour toutes
   les valeurs de défilement X. Lancement : tools/sh2test/run.sh
   ========================================================================= */

#include "amb96_assets.h"
#include "framebuffer.h"
#include "plane.h"
#include "vdp.h"

void plane_draw_line_c(uint32_t offset, const Tilemap *map,
                       const Tileset *tiles, int scrollX, int worldY);
void sys_write(const char *text, int length);

/* Deux frame buffers : celui de la version C, celui de l'assembleur */
static uint8_t fbC[VDP_DRAM_SIZE];
uint8_t sh2test_fb_asm[VDP_DRAM_SIZE];

/* Ce dont la version C a besoin du simulateur : l'écriture 16 bits */
void vdp_dram_write16(uint32_t offset, uint16_t pixels) {
  fbC[offset] = (uint8_t)(pixels >> 8);
  fbC[offset + 1] = (uint8_t)pixels;
}

static void print(const char *s) {
  int n = 0;
  while (s[n])
    n++;
  sys_write(s, n);
}

static void printNumber(unsigned long v) {
  char buf[24];
  int i = sizeof buf;
  buf[--i] = '\0';
  do {
    buf[--i] = (char)('0' + v % 10);
    v /= 10;
  } while (v);
  print(buf + i);
}

static void printSigned(long v) {
  if (v < 0) {
    print("-");
    v = -v;
  }
  printNumber((unsigned long)v);
}

int main(void) {
  const Tilemap *maps[2] = {&asset_plane_a, &asset_plane_b};
  const uint32_t offset = fb_offset(0, 5);
  unsigned long lines = 0, wrong = 0;

  for (int m = 0; m < 2; m++)
    for (int scrollX = -40; scrollX < 1100; scrollX++)
      for (int worldY = -3; worldY < 1030; worldY += 37) {
        plane_draw_line_c(offset, maps[m], &asset_tiles, scrollX, worldY);
        plane_draw_line(offset, maps[m], &asset_tiles, scrollX, worldY);
        lines++;
        for (int x = 0; x < FB_WIDTH; x++)
          if (fbC[offset + x] != sh2test_fb_asm[offset + x]) {
            if (wrong < 5) {
              print("difference : plan ");
              printNumber((unsigned long)m);
              print(", scrollX ");
              printSigned(scrollX);
              print(", worldY ");
              printSigned(worldY);
              print(", x ");
              printNumber((unsigned long)x);
              print("\n");
            }
            wrong++;
            break;
          }
      }

  print("plane_draw_line : ");
  printNumber(lines);
  print(" lignes comparees, ");
  printNumber(wrong);
  print(" differentes\n");
  return wrong != 0;
}
