/* =========================================================================
   plane_draw_line en assembleur SH2 : remplace la version C de plane.c
   quand le projet est compilé avec l'option AMB96_ASM (voir README).

   Même interface que la version C (plane.h), même résultat pixel pour
   pixel. Le C reste la référence : il est lu en premier, il tourne dans le
   simulateur, et cette routine est vérifiée contre lui (tools/sh2test).

     void plane_draw_line(uint32_t offset, const Tilemap *map,
                          const Tileset *tiles, int scrollX, int worldY);

   Convention d'appel de GCC pour SH :
     R4 = offset, R5 = map, R6 = tiles, R7 = scrollX, worldY sur la pile.
     R0-R7 peuvent être modifiés ; R8-R14 doivent être rendus intacts ;
     PR n'est pas touché (aucun appel). MACL est sauvegardé lui aussi :
     l'ABI Renesas le veut, celle de GCC non, autant ne pas parier.

   Même algorithme que le C : la ligne est parcourue tile par tile ; la
   parité du défilement choisit une fois pour toutes la boucle :
     - paire   : MOV.W lu, MOV.W écrit ;
     - impaire : deux MOV.B lus, assemblés en un mot, MOV.W écrit ; le
                 pixel de colonne 15 est gardé pour la tile suivante.
   Les boucles internes utilisent DT et le délai de branchement (BF/S).

   Registres pendant le dessin :
     R0  temporaire         R5  numéros de tile de la rangée (row)
     R1  source (pixels)    R6  ligne voulue dans la tile 0 (tileLine)
     R2  compteur           R7  colonne x dans le plan
     R3  destination (FB)   R8  largeur du plan en pixels - 1 (widthMask)
     R4  pixels restants    R9  pixel gardé << 8 (défilement impair)
     R10 pixel gardé ? (0/1) R11 temporaire
   ========================================================================= */

#include "asm_abi.h"
#include "assets_types.h"

#if TILE_SHIFT != 4
#error "plane_draw_line.S suppose des tiles de 16 x 16 (TILE_SHIFT = 4)"
#endif

/* Les symboles C portent le préfixe du compilateur : '_' pour sh-elf,
   rien pour sh-linux (tests sous qemu). */
#define CONCAT2(a, b) a##b
#define CONCAT(a, b) CONCAT2(a, b)
#define SYMBOL(name) CONCAT(__USER_LABEL_PREFIX__, name)

/* Adresse du frame buffer : celle de la 32X, ou un tableau pour les tests */
#ifdef SH2TEST_FB_SYMBOL
#define FB_BASE SYMBOL(SH2TEST_FB_SYMBOL)
#else
#define FB_BASE FB_DRAM_ADDRESS
#endif

/* Début d'un morceau : la fin de la tile qui contient la colonne x.
   Sortie : R1 = premier pixel, R2 = nombre de pixels (au plus R4).
   Met déjà à jour R4 (pixels restants) et R7 (colonne suivante). */
        .macro  SEGMENT
        mov     r7, r0
        shlr2   r0
        shlr2   r0              /* x >> 4 : tile dans la rangée          */
        add     r0, r0          /* numéros sur 16 bits                    */
        mov.w   @(r0, r5), r1   /* numéro de tile                         */
        extu.w  r1, r1
        shll8   r1              /* x 256 octets par tile                  */
        add     r6, r1
        mov     r7, r0
        and     #15, r0         /* colonne dans la tile                   */
        add     r0, r1          /* R1 = premier pixel du morceau          */
        mov     #16, r2
        sub     r0, r2          /* R2 = pixels jusqu'à la fin de la tile  */
        cmp/gt  r4, r2          /* plus que ce qu'il reste ?              */
        bf      1f
        mov     r4, r2
1:      sub     r2, r4          /* pixels restants                        */
        add     r2, r7
        and     r8, r7          /* colonne suivante, bouclage du plan     */
        .endm

        .text
        .align  2
        .global SYMBOL(plane_draw_line)
SYMBOL(plane_draw_line):
        mov.l   r8, @-r15
        mov.l   r9, @-r15
        mov.l   r10, @-r15
        mov.l   r11, @-r15
        sts.l   macl, @-r15
        mov.l   @(20, r15), r1  /* worldY (5e paramètre)                  */

        /* worldY ramené dans le plan */
        mov.w   @(TILEMAP_HEIGHT_TILES, r5), r0
        shll2   r0
        shll2   r0
        add     #-1, r0
        and     r0, r1

        /* R8 = largeur du plan en pixels - 1, R7 = colonne de départ */
        mov.w   @(TILEMAP_WIDTH_TILES, r5), r0
        extu.w  r0, r9          /* largeur en tiles                       */
        mov     r9, r8
        shll2   r8
        shll2   r8
        add     #-1, r8
        and     r8, r7

        /* R5 = rangée de tiles : cells + (worldY >> 4) x largeur x 2 */
        mov     r1, r0
        shlr2   r0
        shlr2   r0
        mulu.w  r0, r9          /* MACL = rang x largeur (16 x 16 bits)   */
        sts     macl, r0
        add     r0, r0
        mov.l   @(TILEMAP_CELLS, r5), r5
        add     r0, r5

        /* R6 = ligne voulue dans la tile 0 : pixels + (worldY & 15) x 16 */
        mov     r1, r0
        and     #15, r0
        shll2   r0
        shll2   r0
        mov.l   @(TILESET_PIXELS, r6), r6
        add     r0, r6

        /* R3 = destination, R4 = pixels à dessiner */
        mov.l   .Lfb_base, r3
        add     r4, r3
        mov.w   .Lfb_width, r4

        /* Une seule décision de parité pour toute la ligne */
        mov     r7, r0
        tst     #1, r0
        bf      .Lodd

/* ---- Défilement pair : un mot lu, un mot écrit -------------------------- */
.Leven_segment:
        SEGMENT
        shlr    r2              /* mots à copier (morceau de longueur paire) */
2:      mov.w   @r1+, r0
        dt      r2
        mov.w   r0, @r3
        bf/s    2b
        add     #2, r3          /* (délai de branchement)                 */
        tst     r4, r4
        bf      .Leven_segment

.Ldone:
        lds.l   @r15+, macl
        mov.l   @r15+, r11
        mov.l   @r15+, r10
        mov.l   @r15+, r9
        rts
        mov.l   @r15+, r8       /* (délai de branchement)                 */

/* ---- Défilement impair : deux octets lus, assemblés en un mot écrit ---- */
.Lodd:
        mov     #0, r10         /* aucun pixel gardé                      */
.Lodd_segment:
        SEGMENT

        /* Un pixel gardé de la tile précédente : il fait un mot avec le
           premier pixel de celle-ci */
        tst     r10, r10
        bt      3f
        mov.b   @r1+, r0
        extu.b  r0, r0
        or      r9, r0
        mov.w   r0, @r3
        add     #2, r3
        add     #-1, r2

3:      shlr    r2              /* paires ; T = un pixel restera seul     */
        movt    r10             /* ... gardé pour la tile suivante        */
        tst     r2, r2
        bt      5f
4:      mov.b   @r1+, r0        /* pixel de gauche (signe étendu : sans   */
        shll8   r0              /* effet, MOV.W n'écrit que 16 bits)      */
        mov.b   @r1+, r11
        extu.b  r11, r11        /* pixel de droite, sans le signe         */
        or      r11, r0
        dt      r2
        mov.w   r0, @r3
        bf/s    4b
        add     #2, r3          /* (délai de branchement)                 */

5:      tst     r10, r10
        bt      6f
        mov.b   @r1, r9         /* pixel gardé, prêt à devenir poids fort */
        shll8   r9
6:      tst     r4, r4
        bf      .Lodd_segment
        bra     .Ldone
        nop

        .align  2
.Lfb_base:
        .long   FB_BASE
.Lfb_width:
        .word   FB_LINE_PIXELS
