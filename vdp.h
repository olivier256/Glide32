#ifndef VDP_H
#define VDP_H

#include <stdint.h>

/* =========================================================================
   VDP 32X (émulé) : module sans thread.
   ========================================================================= */

#define VDP_SCREEN_WIDTH 320
#define VDP_DRAM_SIZE (128 * 1024)
#define VDP_PALETTE_SIZE 256

/* Frame Buffer Control Register */
#define FBCR_VBLK 0x8000
#define FBCR_HBLK 0x4000
#define FBCR_FS 0x0001

/* ---- Vu des CPU (jeu) -------------------------------------------------- */

/* Frame buffer de dessin (celui qui n'est pas affiché) */
extern uint16_t *vdp_dram;
extern uint16_t vdp_palette[VDP_PALETTE_SIZE];

/* ---- Zone overwrite -------------------------------------------------------
   Sur 32X, le frame buffer de dessin est visible deux fois : en
   2400 0000h (accès normal) et en 2402 0000h (« overwrite image »). Une
   écriture dans la seconde zone ignore les octets nuls : le pixel déjà
   présent est conservé. C'est la transparence des sprites, faite par le
   matériel, quelle que soit la taille de l'accès.

   Comme pour le FBCR, l'effet ne peut pas être obtenu par un simple
   pointeur : ces fonctions tiennent lieu d'écritures à l'adresse
   2402 0000h + offset. offset est compté en octets depuis le début du
   frame buffer de dessin (table de lignes comprise).
   - 8 bits : un pixel.
   - 16 bits : deux pixels ; poids fort = pixel de gauche (adresse la plus
     basse), comme MOV.W sur SH2. offset doit être pair.
   - 32 bits : quatre pixels, même ordre. offset multiple de 4.
   Un accès mal aligné ou hors de la DRAM provoque, comme sur SH2, une
   erreur d'adresse : le simulateur s'arrête avec un message. */
void vdp_overwrite_write8(uint32_t offset, uint8_t pixel);
void vdp_overwrite_write16(uint32_t offset, uint16_t pixels);
void vdp_overwrite_write32(uint32_t offset, uint32_t pixels);

/* ---- Écritures normales ---------------------------------------------------
   Mêmes règles que ci-dessus (offset, ordre des octets, alignement), mais
   dans la zone normale (2400 0000h) : tous les octets sont écrits.
   Pour les pixels : les octets sont rangés dans l'ordre du SH2, comme
   vdp_dram[...] = BE16(...). Pas pour la table de lignes, que le
   simulateur relit comme des mots de l'hôte (voir bigendian.h).
   Sur console : *(volatile uint16_t *)(0x24000000 + offset) = pixels. */
void vdp_dram_write8(uint32_t offset, uint8_t pixel);
void vdp_dram_write16(uint32_t offset, uint16_t pixels);
void vdp_dram_write32(uint32_t offset, uint32_t pixels);

/* ---- Compteurs d'écritures dans le frame buffer ---------------------------
   Le simulateur ne mesure pas le temps SH2, mais il peut compter ce qui
   coûte : les accès au frame buffer, par le bus 16 bits du VDP.
   Une écriture 8 ou 16 bits = un accès ; une écriture 32 bits = deux.
   Seules les écritures par vdp_dram_write*() et vdp_overwrite_write*()
   sont comptées (pas celles faites directement par le pointeur vdp_dram).
   Une « image » va d'un échange de frame buffer au suivant : ce sont les
   écritures qui ont servi à dessiner l'image qui vient d'être affichée. */

enum { VDP_AREA_NORMAL, VDP_AREA_OVERWRITE, VDP_AREAS };
enum { VDP_WRITE_8, VDP_WRITE_16, VDP_WRITE_32, VDP_WRITE_SIZES };

typedef struct {
  uint32_t writes[2][VDP_AREAS][VDP_WRITE_SIZES]; /* [cpu][zone][taille] */
} vdp_frame_writes;

/* Accès au bus pour un CPU : n8 + n16 + 2 x n32, toutes zones */
uint32_t vdp_bus_accesses(const vdp_frame_writes *frame, int cpu);

typedef struct {
  unsigned long frames;          /* images terminées depuis le début */
  vdp_frame_writes last;         /* la dernière image terminée */
  uint32_t maxBusAccesses[2];    /* pire image depuis la dernière remise à 0 */
  uint64_t totalBusAccesses[2];  /* depuis le début */
} vdp_write_stats;

/* resetMax : remet maxBusAccesses à zéro après lecture (pics par période) */
void vdp_get_write_stats(vdp_write_stats *stats, int resetMax);

/* Affiche les compteurs sur stderr, sur deux lignes préfixées par 'label',
   et remet les maxima à zéro. Utilisée par les deux plates-formes. */
void vdp_print_write_stats(const char *label);

/* Registres */
extern volatile uint16_t *const bitmapModeRegister;
uint16_t vdp_read_fbcr(void);
void vdp_write_fbcr(uint16_t value);

/* Nombre de lignes affichées */
int vdp_screen_height(void);

/* ---- Vu de la plate-forme ---------------------------------------------- */

void vdp_init(int screenHeight);

/* Appelées par la synchro vidéo aux frontières de lignes */
void vdp_begin_vblank(void);
void vdp_end_vblank(void);

/* Décode une ligne affichée du frame buffer affiché vers RGB
   (VDP_SCREEN_WIDTH pixels, 0x00RRGGBB). */
void vdp_render_line(int line, uint32_t *rgb);

/* Diagnostics : nombre de VBlank, et nombre de VBlank vus par chaque CPU
   (au moins une lecture du FBCR avec VBLK = 1 pendant ce VBlank). */
typedef struct {
  unsigned long vblanks;
  unsigned long seen[2];
} vdp_stats;

void vdp_get_stats(vdp_stats *stats);

#endif
