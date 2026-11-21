/* =========================================================================
   Banc d'essai des routines assembleur SH2, sous qemu-sh4eb (SH gros-
   boutiste, mode utilisateur Linux). Compile la version C de chaque
   routine (renommée ..._c) et la version assembleur dans le même
   programme, et compare leurs résultats :
   - plane_draw_line : toutes les valeurs de défilement X ;
   - sprite_copy_row : toutes les longueurs et positions de départ, dans
     chaque ligne de chaque sprite, destination paire et impaire.
   Lancement : tools/sh2test/run.sh
   ========================================================================= */

#include "amb96_assets.h"
#include "framebuffer.h"
#include "plane.h"
#include "sprite.h"
#include "vdp.h"

void plane_draw_line_c(uint32_t offset, const Tilemap *map,
                       const Tileset *tiles, int scrollX, int worldY);
void sprite_copy_row_c(uint32_t offset, const uint8_t *src, int n);
void sys_write(const char *text, int length);

/* Deux frame buffers : celui de la version C, celui de l'assembleur */
static uint8_t fbC[VDP_DRAM_SIZE];
/* Alignés comme les vraies zones du frame buffer (2400 0000h, 2402 0000h) :
   les routines testent la parité de l'adresse, le C celle de l'offset. */
uint8_t sh2test_fb_asm[VDP_DRAM_SIZE] __attribute__((aligned(4)));

/* Zone overwrite : celle de la version C, celle de l'assembleur */
static uint8_t overwriteC[VDP_DRAM_SIZE];
uint8_t sh2test_overwrite_asm[VDP_DRAM_SIZE] __attribute__((aligned(4)));

/* Ce dont les versions C ont besoin du simulateur */
void vdp_dram_write16(uint32_t offset, uint16_t pixels) {
  fbC[offset] = (uint8_t)(pixels >> 8);
  fbC[offset + 1] = (uint8_t)pixels;
}

/* La zone overwrite du banc est un tableau ordinaire : un MOV.W de
   l'assembleur y écrit aussi ses octets nuls, là où le VDP les ignorerait.
   La version C écrit donc ici telle quelle, sans imiter le VDP : on
   compare ce que les deux versions écrivent (quels mots, quels octets),
   pas l'image finale. L'effet de la zone overwrite est le travail du
   simulateur, vérifié par ailleurs. */
void vdp_overwrite_write8(uint32_t offset, uint8_t pixel) {
  overwriteC[offset] = pixel;
}

void vdp_overwrite_write16(uint32_t offset, uint16_t pixels) {
  overwriteC[offset] = (uint8_t)(pixels >> 8);
  overwriteC[offset + 1] = (uint8_t)pixels;
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

/* sprite_copy_row : chaque ligne de chaque sprite, toutes les colonnes de
   départ et toutes les longueurs, vers une destination paire puis impaire.
   Les deux zones sont remplies du même motif avant chaque copie, pour voir
   aussi ce qui serait écrit en trop. */
static unsigned long testSpriteCopyRow(unsigned long *copies) {
  static const Sprite *sprites[] = {&asset_sonic, &asset_corner_tl,
                                    &asset_corner_tr, &asset_corner_bl,
                                    &asset_corner_br};
  unsigned long wrong = 0;
  *copies = 0;

  for (int s = 0; s < 5; s++) {
    const Sprite *sp = sprites[s];
    for (int row = 0; row < sp->height; row += 3)
      for (int first = 0; first < sp->width; first++)
        for (int n = 1; first + n <= sp->width; n++)
          for (uint32_t offset = 1000; offset < 1002; offset++) {
            for (uint32_t i = offset - 4; i < offset + n + 4; i++)
              overwriteC[i] = sh2test_overwrite_asm[i] = (uint8_t)(i * 7 + 1);

            const uint8_t *src = sp->pixels + row * sp->width + first;
            sprite_copy_row_c(offset, src, n);
            sprite_copy_row(offset, src, n);
            (*copies)++;

            for (uint32_t i = offset - 4; i < offset + n + 4; i++)
              if (overwriteC[i] != sh2test_overwrite_asm[i]) {
                if (wrong < 5) {
                  print("difference : sprite ");
                  printNumber((unsigned long)s);
                  print(", ligne ");
                  printNumber((unsigned long)row);
                  print(", colonne ");
                  printNumber((unsigned long)first);
                  print(", n ");
                  printNumber((unsigned long)n);
                  print(", offset ");
                  printNumber(offset);
                  print("\n");
                }
                wrong++;
                break;
              }
          }
  }
  return wrong;
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

  unsigned long copies;
  unsigned long spriteWrong = testSpriteCopyRow(&copies);
  print("sprite_copy_row : ");
  printNumber(copies);
  print(" copies comparees, ");
  printNumber(spriteWrong);
  print(" differentes\n");

  return wrong != 0 || spriteWrong != 0;
}
