#ifndef GLIDE_H
#define GLIDE_H

#include "fixed.h"
#include <stdint.h>

/* =========================================================================
   Glide 32X : un sous-ensemble de Glide 3 (3Dfx), pour les deux SH2.
   Bibliothèque du jeu, C99, sans thread.

   Modèle : Scan Line Interleave (SLI), comme deux Voodoo2 couplées.
   - Le master joue le rôle du CPU hôte ET de la première carte : chaque
     commande est copiée dans une FIFO en SDRAM, annoncée au slave, puis
     exécutée aussitôt pour les lignes PAIRES.
   - Le slave joue le rôle de la seconde carte : il lit la FIFO et exécute
     les mêmes commandes pour les lignes IMPAIRES.
   Glide ne transforme rien : les sommets arrivent en coordonnées écran.

   Toutes les fonctions gr* sont réservées au master, sauf grSlaveLoop().
   La FIFO et le z-buffer (320 x 240 x 16 bits, 150 Ko) sont des variables
   statiques de glide.c : sur 32X, l'éditeur de liens les place en SDRAM.

   Registres COMM : comme pour la BootROM, comm[0] et comm[1] appartiennent
   au master, comm[2] et comm[3] au slave. Chaque CPU ne fait que LIRE les
   registres de l'autre. Glide les occupe tous les quatre :
     comm[0] : état du master (GR_COMM_READY quand Glide est ouvert)
     comm[1] : position d'écriture dans la FIFO
     comm[2] : position de lecture dans la FIFO
     comm[3] : état du slave (GR_COMM_READY quand il attend les commandes)
   comm[4] à comm[7] restent au jeu.
   ========================================================================= */

/* Valeur « prêt » des registres d'état. Elle diffère de « M_OK » et
   « S_OK », que la BootROM laisse dans comm[0] à comm[3]. */
#define GR_COMM_READY 0x4752 /* "GR" */

/* ---- Types ------------------------------------------------------------- */

/* Virgule fixe 16.16 (remplace FxFloat de Glide) : alias de fixed.h,
   pour garder les noms de Glide. */
typedef Fixed GrFixed;

#define GR_FIXED_ONE FIXED_ONE
#define GR_FIXED_HALF FIXED_HALF
#define GR_FIXED(i) FIXED(i)

/* Couleur : index dans la palette 8 bits du mode Packed Pixel
   (remplace GrColor_t, qui est en RGBA 32 bits dans Glide). */
typedef uint8_t GrColor_t;

/* Profondeur : 1/z en virgule fixe 16.16 non signée, dans [0 ; 65535].
   Glide appelle ce paramètre « z », mais c'est bien 1/z (ooz) : seul 1/z
   varie linéairement à l'écran. Typiquement ooz = 65535 / z, avec z >= 1
   au plan proche. Plus ooz est grand, plus le point est proche. */
typedef uint32_t GrDepth;

#define GR_DEPTH(i) ((GrDepth)(i) << 16)

/* Sommet en coordonnées écran, origine en haut à gauche (équivalent de
   grSstOrigin(GR_ORIGIN_UPPER_LEFT), seule origine prévue).
   Le centre du pixel (x, y) est en (x + 0,5 ; y + 0,5).
   x et y doivent rester dans ]-4096 ; 4096[ pixels : au-delà, les calculs
   en 32 bits débordent (Glide impose aussi un domaine aux sommets).
   Disposition fixe : pas de grVertexLayout(). */
typedef struct {
  GrFixed x;
  GrFixed y;
  GrDepth ooz; /* pour le z-buffer ; ignoré sans tampon de profondeur */
  /* GrFixed oow;  plus tard, pour le w-buffer */
} GrVertex;

/* Modes du tampon de profondeur (grDepthBufferMode) */
typedef int GrDepthBufferMode_t;
#define GR_DEPTHBUFFER_DISABLE 0 /* par défaut */
#define GR_DEPTHBUFFER_ZBUFFER 1 /* z-buffer 16 bits : partie entière de ooz */
/* #define GR_DEPTHBUFFER_WBUFFER 2   plus tard : flottant 16 bits de oow */

/* Fonctions de comparaison (grDepthBufferFunction), mêmes valeurs que Glide.
   Le test passe si : profondeur du pixel <fonction> profondeur stockée. */
typedef int GrCmpFnc_t;
#define GR_CMP_NEVER 0
#define GR_CMP_LESS 1 /* par défaut */
#define GR_CMP_EQUAL 2
#define GR_CMP_LEQUAL 3
#define GR_CMP_GREATER 4
#define GR_CMP_NOTEQUAL 5
#define GR_CMP_GEQUAL 6
#define GR_CMP_ALWAYS 7

/* ---- Ouverture --------------------------------------------------------- */

/* Équivalent de grSstWinOpen() : passe le VDP en Packed Pixel, initialise
   les tables de lignes des deux frame buffers, prépare la FIFO, libère le
   slave et attend qu'il soit prêt. À appeler une fois, sur le master,
   avant tout autre gr*. La palette reste à la charge du jeu. */
void grSstWinOpen(void);

/* ---- Boucle du slave --------------------------------------------------- */

/* Le slave attend grSstWinOpen(), puis exécute la FIFO, pour les lignes
   impaires, jusqu'à l'arrêt de l'hôte. Ne rend la main qu'à l'arrêt :
   le slave est entièrement consacré au rendu, comme une carte Voodoo. */
void grSlaveLoop(void);

/* ---- États (commandes de la FIFO) -------------------------------------- */

/* Couleur des triangles suivants (flat shading).
   Dans Glide : grConstantColorValue() + grColorCombine(... CONSTANT ...). */
void grConstantColorValue(GrColor_t color);

/* Fenêtre de découpage : seuls les pixels de [minx, maxx[ x [miny, maxy[
   sont tracés. Ramenée à l'écran si elle déborde. Par défaut : l'écran. */
void grClipWindow(int minx, int miny, int maxx, int maxy);

/* Active le tampon de profondeur. Par défaut : GR_DEPTHBUFFER_DISABLE. */
void grDepthBufferMode(GrDepthBufferMode_t mode);

/* Fonction de comparaison. Par défaut : GR_CMP_LESS, comme Glide.
   ATTENTION : avec le z-buffer, ooz DIMINUE quand z augmente ; il faut
   donc GR_CMP_GREATER et un effacement à 0 (exemple 7.1 du guide Glide).
   GR_CMP_LESS convient au futur w-buffer, où les grandes valeurs sont
   lointaines. */
void grDepthBufferFunction(GrCmpFnc_t function);

/* Autorise (enable != 0, par défaut) ou interdit l'écriture dans le tampon
   de profondeur. Le test a lieu dans les deux cas. */
void grDepthMask(int enable);

/* ---- Tracé (commandes de la FIFO) -------------------------------------- */

/* Efface la fenêtre de découpage du back buffer, et celle du tampon de
   profondeur s'il est actif et que son écriture est autorisée (grDepthMask).
   depth est la valeur écrite dans le z-buffer (0 = le plus lointain avec
   ooz). (Glide : grBufferClear(color, alpha, depth) ; pas d'alpha ici.) */
void grBufferClear(GrColor_t color, uint16_t depth);

/* Trace un triangle plein, à la couleur courante, avec le test de
   profondeur si le z-buffer est actif (ooz interpolé sur le triangle).
   Les sommets sont copiés dans la FIFO : l'appelant peut les réutiliser
   dès le retour. Règle de remplissage de Glide : un pixel est tracé si son
   centre est dans le triangle ; arêtes gauches et hautes incluses, droites
   et basses exclues. Ordre des sommets indifférent (pas de culling). */
void grDrawTriangle(const GrVertex *a, const GrVertex *b, const GrVertex *c);

/* ---- Synchronisation --------------------------------------------------- */

/* Chaque commande est annoncée au slave dès qu'elle est écrite : grFlush()
   n'a rien à faire. Elle existe pour la fidélité à Glide, dont le manuel
   dit la même chose du matériel de l'époque. */
void grFlush(void);

/* Attend que le slave ait exécuté toutes les commandes. */
void grFinish(void);

/* Affiche l'image : grFinish(), demande de flip (hors VBlank), puis attente
   du VBlank. Au retour, on dessine dans l'autre buffer, qui contient
   l'avant-dernière image : il faut l'effacer (grBufferClear).
   swapInterval : seule la valeur 1 est prévue pour l'instant. */
void grBufferSwap(int swapInterval);

/* ---- Diagnostic (extension, absente de Glide) -------------------------- */

/* Si enable != 0, les triangles tracés par le master prennent la couleur
   masterColor et ceux du slave slaveColor, quelle que soit la couleur
   courante : un triangle « zébré » prouve que le SLI fonctionne.
   grBufferClear n'est pas concerné. Commande de la FIFO, comme un état. */
void grSliDebug(int enable, GrColor_t masterColor, GrColor_t slaveColor);

#endif
