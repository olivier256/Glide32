/* Point d'entrée minimal pour qemu-sh4eb (Linux, mode utilisateur) :
   appelle main, puis quitte avec son code de retour. */
#define CONCAT2(a, b) a##b
#define CONCAT(a, b) CONCAT2(a, b)
#define SYMBOL(name) CONCAT(__USER_LABEL_PREFIX__, name)

        .text
        .align  2
        .global _start
_start:
        mov.l   .Lmain, r0
        jsr     @r0
        nop
        mov     r0, r4          /* code de retour */
        mov     #1, r3          /* appel système exit */
        trapa   #0x11
        .align  2
.Lmain: .long   SYMBOL(main)

/* write(1, texte, longueur) */
        .global SYMBOL(sys_write)
SYMBOL(sys_write):
        mov     r5, r6
        mov     r4, r5
        mov     #1, r4
        mov     #4, r3          /* appel système write */
        trapa   #0x13
        rts
        nop

/* sprite.c contient aussi sprite_draw, qui divise (fonction d'aide de
   libgcc). Le banc ne l'appelle pas, et la libgcc de cette chaîne est
   petit-boutiste : une souche suffit pour l'édition de liens. */
        .global SYMBOL(__sdivsi3_i4i)
SYMBOL(__sdivsi3_i4i):
        rts
        mov     #0, r0
