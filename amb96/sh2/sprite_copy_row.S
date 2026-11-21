/* =========================================================================
   sprite_copy_row en assembleur SH2 : remplace la version C de sprite.c
   quand le projet est compilé avec l'option AMB96_ASM (voir README).

   Même interface que la version C (sprite.h), même résultat ; vérifiée
   contre elle par tools/sh2test.

     void sprite_copy_row(uint32_t offset, const uint8_t *src, int n);

   R4 = offset (zone overwrite), R5 = src, R6 = n. Aucun registre à
   sauvegarder : seuls R0-R7 sont utilisés, et rien n'est appelé.

   Comme en C : un premier pixel seul si la destination est impaire, puis
   des paires, puis un dernier pixel seul ; un mot ou un pixel transparent
   n'est pas écrit. Pour les paires, la destination est toujours paire ;
   la parité de la source choisit la boucle :
     - source paire   : un MOV.W lu, testé, écrit ;
     - source impaire : deux MOV.B lus, assemblés, testés, écrits.
   Les écritures se font par adressage indexé @(R0,R5) : R5 avance seul
   (lectures en @R5+), R0 garde l'écart constant destination - source.

   Registres :
     R0 écart destination - source     R4 offset
     R1 pixels                         R5 source
     R2 nombre de paires (compteur)    R6 n, puis pixel de droite (impaire)
     R3 adresse de destination         R7 1 s'il reste un pixel seul à la fin
   ========================================================================= */

#include "asm_abi.h"

#define CONCAT2(a, b) a##b
#define CONCAT(a, b) CONCAT2(a, b)
#define SYMBOL(name) CONCAT(__USER_LABEL_PREFIX__, name)

#ifdef SH2TEST_OVERWRITE_SYMBOL
#define OVERWRITE_BASE SYMBOL(SH2TEST_OVERWRITE_SYMBOL)
#else
#define OVERWRITE_BASE FB_OVERWRITE_ADDRESS
#endif

        .text
        .align  2
        .global SYMBOL(sprite_copy_row)
SYMBOL(sprite_copy_row):
        mov.l   .Loverwrite, r3
        add     r4, r3          /* R3 = adresse de destination            */

        /* Destination impaire : un premier pixel seul */
        mov     r3, r0
        tst     #1, r0
        bt      1f
        mov.b   @r5+, r1
        tst     r1, r1
        bt/s    0f              /* transparent : pas d'écriture           */
        add     #-1, r6         /* (délai) un pixel de moins              */
        mov.b   r1, @r3
0:      add     #1, r3

1:      mov     r3, r0
        sub     r5, r0          /* R0 = écart destination - source        */
        mov     r6, r2
        shlr    r2              /* R2 = paires ; T = un pixel restera seul */
        movt    r7
        tst     r2, r2
        bt      .Llast

        mov     r5, r1
        shlr    r1              /* T = source impaire                     */
        bt      .Lodd_source

/* ---- Source paire : un mot lu, testé, écrit ------------------------------
   Le mot suivant est lu dans le délai du branchement de boucle : la
   lecture d'après la dernière paire est de trop (2 octets au-delà de la
   ligne, sans effet), et R5 est corrigé après la boucle. */
        add     #-2, r0         /* R5 aura déjà avancé à l'écriture       */
        mov.w   @r5+, r1        /* première paire                         */
2:      tst     r1, r1
        bt/s    3f              /* transparente : pas d'écriture          */
        dt      r2              /* (délai) compteur, pour les deux chemins */
        mov.w   r1, @(r0, r5)
3:      bf/s    2b
        mov.w   @r5+, r1        /* (délai) paire suivante                 */
        add     #-2, r5         /* une lecture de trop                    */
        bra     .Llast
        add     #2, r0          /* (délai) R0 = écart, de nouveau         */

/* ---- Source impaire : deux octets lus, assemblés, testés, écrits --------
   Le pixel de gauche garde son extension de signe après SHLL8 : les bits
   parasites n'apparaissent que s'il est non nul, et alors le mot n'est de
   toute façon pas transparent ; MOV.W n'écrit que les 16 bits du bas. */
.Lodd_source:
        add     #-2, r0
4:      mov.b   @r5+, r1        /* pixel de gauche                        */
        shll8   r1
        mov.b   @r5+, r6        /* pixel de droite                        */
        extu.b  r6, r6
        or      r6, r1
        tst     r1, r1
        bt/s    5f              /* transparente : pas d'écriture          */
        dt      r2              /* (délai)                                */
        mov.w   r1, @(r0, r5)
5:      bf      4b
        add     #2, r0

/* ---- Dernier pixel seul, s'il n'est pas transparent ------------------- */
.Llast:
        tst     r7, r7
        bt      6f
        mov.b   @r5, r1
        tst     r1, r1
        bt      6f
        mov.b   r1, @(r0, r5)
6:      rts
        nop

        .align  2
.Loverwrite:
        .long   OVERWRITE_BASE
