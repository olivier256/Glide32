#!/bin/sh
# =========================================================================
# Vérifie les routines assembleur SH2 contre la version C, sous qemu.
# Il faut : un compilateur SH (sh4-linux-gnu-gcc) et qemu-sh4eb (qemu en
# mode utilisateur, disponible sous Linux, pas sous macOS : utiliser une
# VM ou Docker).
# Les routines .S sont assemblées en SH2 strict (--isa=sh2) : une
# instruction SH3/SH4 y serait refusée. Le C du banc d'essai est compilé
# pour SH4 sans FPU (ce compilateur ne sait pas faire de SH2) : il ne sert
# que de référence, en gros-boutiste comme la 32X.
#   Debian/Ubuntu : apt install gcc-sh4-linux-gnu qemu-user
# Usage : tools/sh2test/run.sh   (depuis la racine du dépôt)
# =========================================================================
set -e

CC=${SH_CC:-sh4-linux-gnu-gcc}
QEMU=${SH_QEMU:-qemu-sh4eb}
OUT=build/sh2test
mkdir -p "$OUT"

# Les images, converties par bmp2c compilé pour l'hôte
cc -std=c99 -O2 -Iamb96 -o "$OUT/bmp2c" tools/bmp2c.c
"$OUT/bmp2c" amb96/assets/assets.txt "$OUT/amb96_assets"

FLAGS="-m4-nofpu -mb -O2 -std=c99 -ffreestanding -fno-builtin -Wall -I. -Iamb96 -I$OUT"
ASFLAGS="-mb -Wa,--isa=sh2 -Wa,--noexecstack -I. -Iamb96"

# Version C, renommée ; version assembleur, écrivant dans sh2test_fb_asm
$CC $FLAGS -Dplane_draw_line=plane_draw_line_c -c amb96/plane.c -o "$OUT/plane_c.o"
$CC $ASFLAGS -DSH2TEST_FB_SYMBOL=sh2test_fb_asm -c amb96/sh2/plane_draw_line.S -o "$OUT/plane_asm.o"
$CC $FLAGS -Dsprite_copy_row=sprite_copy_row_c -c amb96/sprite.c -o "$OUT/sprite_c.o"
$CC $ASFLAGS -DSH2TEST_OVERWRITE_SYMBOL=sh2test_overwrite_asm -c amb96/sh2/sprite_copy_row.S -o "$OUT/sprite_asm.o"
$CC $FLAGS -c tools/sh2test/sh2test.c -o "$OUT/sh2test.o"
$CC $ASFLAGS -c tools/sh2test/start.S -o "$OUT/start.o"
$CC $FLAGS -c "$OUT/amb96_assets.c" -o "$OUT/assets.o"
# Édition de liens directe : le pilote gcc de cette chaîne ne connaît que
# le petit-boutiste pour lier, mais ld sait produire du gros-boutiste.
LD=${SH_LD:-sh4-linux-gnu-ld}
$LD -EB -static -o "$OUT/sh2test" "$OUT/start.o" "$OUT/sh2test.o" \
  "$OUT/plane_c.o" "$OUT/plane_asm.o" "$OUT/sprite_c.o" "$OUT/sprite_asm.o" \
  "$OUT/assets.o"

$QEMU "$OUT/sh2test"
