#include "glide.h"
#include "interface.h"
#include "platform.h" /* platform_running() uniquement */
#include "sh2.h"
#include "vdp.h"

/* =========================================================================
   Registres COMM (voir glide.h)
   ========================================================================= */

#define COMM_MASTER_STATE 0 /* écrit par le master */
#define COMM_WRITE_POS 1    /* écrit par le master */
#define COMM_READ_POS 2     /* écrit par le slave */
#define COMM_SLAVE_STATE 3  /* écrit par le slave */

/* =========================================================================
   FIFO de commandes, en SDRAM

   Un anneau de mots de 32 bits. Le master seul écrit les commandes et
   comm[1] ; le slave seul écrit comm[2].
   - Lecture == écriture : la FIFO est vide.
   - On laisse toujours un mot libre entre l'écriture et la lecture : sinon
     une FIFO pleine ne se distinguerait pas d'une FIFO vide.
   - Une commande n'est jamais coupée par la fin de l'anneau : quand elle
     ne tient pas, le master écrit CMD_WRAP, qui renvoie le lecteur au début.
     Le dernier mot de l'anneau est réservé à ce CMD_WRAP.
   Les positions sont en mots : elles tiennent dans un registre COMM.
   ========================================================================= */

#define FIFO_SIZE 4096 /* mots de 32 bits, soit 16 Ko */

static uint32_t fifo[FIFO_SIZE];
static uint16_t writePos = 0; /* position d'écriture, propre au master */

/* Commandes. Chacune commence par son code, suivi de ses paramètres. */
enum {
  CMD_WRAP = 1,  /* retour au début de l'anneau                  1 mot  */
  CMD_COLOR,     /* couleur                                      2 mots */
  CMD_CLIP,      /* minx, miny, maxx, maxy                       5 mots */
  CMD_CLEAR,     /* couleur, profondeur                          3 mots */
  CMD_TRIANGLE,  /* ax, ay, bx, by, cx, cy                       7 mots */
  CMD_SLI_DEBUG  /* actif, couleur master, couleur slave         4 mots */
};

/* =========================================================================
   Rasteriseur : un par CPU, chacun avec ses états et sa parité de lignes
   ========================================================================= */

typedef struct {
  int parity; /* 0 : lignes paires (master), 1 : impaires (slave) */
  GrColor_t color;
  int clipMinX, clipMinY, clipMaxX, clipMaxY;
  int sliDebug;
  GrColor_t sliColor[2];
} Rasterizer;

static Rasterizer masterRaster;
static Rasterizer slaveRaster;

static void resetRasterizer(Rasterizer *r, int parity);
static int execute(Rasterizer *r, const uint32_t *cmd);
static void clearBuffer(const Rasterizer *r, GrColor_t color);
static void drawTriangle(const Rasterizer *r, const uint32_t *v);

static uint32_t *reserve(int count);
static void submit(int count);

static void lineTableInit(int screenHeight);
static void waitForVBlank(void);
static void waitForDisplay(void);

/* =========================================================================
   Ouverture (master)
   ========================================================================= */

void grSstWinOpen(void) {
  const int screenHeight = vdp_screen_height();

  /* Blank Mode : le flip s'applique tout de suite, on peut préparer les
     tables de lignes des deux frame buffers. */
  *bitmapModeRegister = (*bitmapModeRegister & 0xFFFC);
  lineTableInit(screenHeight);
  vdp_write_fbcr(vdp_read_fbcr() ^ FBCR_FS);
  lineTableInit(screenHeight);

  /* Packed Pixel Mode */
  *bitmapModeRegister = (*bitmapModeRegister & 0xFFFC) | 0x0001;

  /* FIFO vide, puis on libère le slave */
  writePos = 0;
  resetRasterizer(&masterRaster, 0);
  comm[COMM_WRITE_POS] = 0;
  sh2_sync();
  comm[COMM_MASTER_STATE] = GR_COMM_READY;

  /* Le slave a remis sa position de lecture à 0 avant de se dire prêt :
     avant cela, comm[2] contient n'importe quoi (« S_OK » sur 32X). */
  while (comm[COMM_SLAVE_STATE] != GR_COMM_READY)
    if (!platform_running())
      return;
  sh2_sync();
}

/* =========================================================================
   Boucle du slave
   ========================================================================= */

void grSlaveLoop(void) {
  while (comm[COMM_MASTER_STATE] != GR_COMM_READY)
    if (!platform_running())
      return;
  sh2_sync();

  resetRasterizer(&slaveRaster, 1);
  uint16_t readPos = 0;
  comm[COMM_READ_POS] = readPos;
  sh2_sync();
  comm[COMM_SLAVE_STATE] = GR_COMM_READY;

  while (platform_running()) {
    if (comm[COMM_WRITE_POS] == readPos)
      continue; /* FIFO vide */

    /* La commande a été écrite avant d'être annoncée : la relire après
       la position. */
    sh2_sync();

    int count = execute(&slaveRaster, fifo + readPos);
    readPos = count ? (uint16_t)(readPos + count) : 0; /* 0 : CMD_WRAP */

    /* Fini de lire cette commande avant de rendre sa place au master */
    sh2_sync();
    comm[COMM_READ_POS] = readPos;
  }
}

/* =========================================================================
   Commandes (master)
   ========================================================================= */

void grConstantColorValue(GrColor_t color) {
  uint32_t *p = reserve(2);
  *p++ = CMD_COLOR;
  *p = color;
  submit(2);
}

void grClipWindow(int minx, int miny, int maxx, int maxy) {
  const int w = VDP_SCREEN_WIDTH;
  const int h = vdp_screen_height();

  if (minx < 0) minx = 0;
  if (miny < 0) miny = 0;
  if (maxx > w) maxx = w;
  if (maxy > h) maxy = h;
  if (maxx < minx) maxx = minx;
  if (maxy < miny) maxy = miny;

  uint32_t *p = reserve(5);
  *p++ = CMD_CLIP;
  *p++ = (uint32_t)minx;
  *p++ = (uint32_t)miny;
  *p++ = (uint32_t)maxx;
  *p = (uint32_t)maxy;
  submit(5);
}

void grBufferClear(GrColor_t color, uint16_t depth) {
  uint32_t *p = reserve(3);
  *p++ = CMD_CLEAR;
  *p++ = color;
  *p = depth;
  submit(3);
}

void grDrawTriangle(const GrVertex *a, const GrVertex *b, const GrVertex *c) {
  uint32_t *p = reserve(7);
  *p++ = CMD_TRIANGLE;
  *p++ = (uint32_t)a->x;
  *p++ = (uint32_t)a->y;
  *p++ = (uint32_t)b->x;
  *p++ = (uint32_t)b->y;
  *p++ = (uint32_t)c->x;
  *p = (uint32_t)c->y;
  submit(7);
}

void grSliDebug(int enable, GrColor_t masterColor, GrColor_t slaveColor) {
  uint32_t *p = reserve(4);
  *p++ = CMD_SLI_DEBUG;
  *p++ = enable ? 1 : 0;
  *p++ = masterColor;
  *p = slaveColor;
  submit(4);
}

/* =========================================================================
   Synchronisation (master)
   ========================================================================= */

void grFlush(void) {}

void grFinish(void) {
  while (comm[COMM_READ_POS] != writePos)
    if (!platform_running())
      return;
  sh2_sync();
}

void grBufferSwap(int swapInterval) {
  (void)swapInterval; /* toujours 1 pour l'instant */

  /* Le slave a fini l'image */
  grFinish();

  /* Garde-fou : la demande de flip doit tomber hors VBlank, sinon elle
     s'appliquerait tout de suite et l'on pourrait changer deux fois de
     buffer pendant le même VBlank. */
  waitForDisplay();
  vdp_write_fbcr(vdp_read_fbcr() ^ FBCR_FS);
  waitForVBlank();

  /* Le flip a eu lieu : vdp_dram désigne le nouveau buffer de dessin.
     Aucune commande de l'image suivante n'a encore été écrite, le slave
     ne peut donc pas tracer dans l'ancien. C'est ce qui dispense de faire
     passer l'échange de buffers par la FIFO. */
}

/* =========================================================================
   FIFO (master)
   ========================================================================= */

/* Mots libres, en laissant toujours un mot entre écriture et lecture */
static int freeWords(void) {
  int readPos = comm[COMM_READ_POS];
  return (readPos - writePos - 1 + FIFO_SIZE) % FIFO_SIZE;
}

/* Attend que count mots soient libres. À l'arrêt de l'hôte, n'attend plus :
   on écrase alors des commandes que le slave ne lira jamais. */
static void waitForSpace(int count) {
  while (freeWords() < count)
    if (!platform_running())
      break;

  /* Le slave a fini de lire ces mots : ne pas les réécrire avant */
  sh2_sync();
}

/* Renvoie l'adresse où écrire une commande de count mots */
static uint32_t *reserve(int count) {
  if (writePos + count > FIFO_SIZE - 1) {
    /* Ne tient pas avant la fin : CMD_WRAP occupe la fin de l'anneau, et
       le lecteur ne doit être ni dans cette fin ni au début. */
    waitForSpace(FIFO_SIZE - writePos);
    fifo[writePos] = CMD_WRAP;
    writePos = 0;
    sh2_sync();
    comm[COMM_WRITE_POS] = writePos;
  }

  waitForSpace(count);
  return fifo + writePos;
}

/* La commande est écrite : on l'annonce au slave, PUIS le master trace ses
   lignes. Les deux CPU travaillent ainsi en même temps. */
static void submit(int count) {
  const uint32_t *cmd = fifo + writePos;

  writePos = (uint16_t)(writePos + count);
  sh2_sync();
  comm[COMM_WRITE_POS] = writePos;

  execute(&masterRaster, cmd);
}

/* =========================================================================
   Exécution d'une commande (master et slave)
   ========================================================================= */

static void resetRasterizer(Rasterizer *r, int parity) {
  r->parity = parity;
  r->color = 0;
  r->clipMinX = 0;
  r->clipMinY = 0;
  r->clipMaxX = VDP_SCREEN_WIDTH;
  r->clipMaxY = vdp_screen_height();
  r->sliDebug = 0;
  r->sliColor[0] = 0;
  r->sliColor[1] = 0;
}

/* Renvoie le nombre de mots de la commande, ou 0 pour CMD_WRAP */
static int execute(Rasterizer *r, const uint32_t *cmd) {
  switch (*cmd) {
  case CMD_COLOR:
    r->color = (GrColor_t)cmd[1];
    return 2;

  case CMD_CLIP:
    r->clipMinX = (int)cmd[1];
    r->clipMinY = (int)cmd[2];
    r->clipMaxX = (int)cmd[3];
    r->clipMaxY = (int)cmd[4];
    return 5;

  case CMD_CLEAR:
    clearBuffer(r, (GrColor_t)cmd[1]);
    return 3;

  case CMD_TRIANGLE:
    drawTriangle(r, cmd + 1);
    return 7;

  case CMD_SLI_DEBUG:
    r->sliDebug = (int)cmd[1];
    r->sliColor[0] = (GrColor_t)cmd[2];
    r->sliColor[1] = (GrColor_t)cmd[3];
    return 4;

  default: /* CMD_WRAP */
    return 0;
  }
}

/* =========================================================================
   Tracé
   ========================================================================= */

/* Première ligne >= y qui appartient à ce CPU */
static int firstOwnLine(const Rasterizer *r, int y) {
  return y + ((y ^ r->parity) & 1);
}

/* Plus petit entier >= v (v en 16.16).
   Le décalage à droite d'un nombre négatif est arithmétique avec GCC et
   Clang, sur SH2 comme sur l'hôte. */
static int ceilFixed(GrFixed v) { return (v + GR_FIXED_ONE - 1) >> 16; }

/* Remplit les pixels [x0, x1[ de la ligne y, octet par octet */
static void fillSpan(int y, int x0, int x1, GrColor_t color) {
  uint8_t *p = (uint8_t *)vdp_dram + vdp_dram[y] * 2 + x0;
  uint8_t *end = p + (x1 - x0);

  while (p < end)
    *p++ = color;
}

static void clearBuffer(const Rasterizer *r, GrColor_t color) {
  for (int y = firstOwnLine(r, r->clipMinY); y < r->clipMaxY; y += 2)
    fillSpan(y, r->clipMinX, r->clipMaxX, color);
}

/* ---- Arêtes ---------------------------------------------------------------
   Une arête va toujours de son sommet le plus haut à son sommet le plus
   bas. Elle couvre les lignes dont le centre (y + 0,5) est dans
   [haut, bas[ : la ligne du haut est incluse, celle du bas exclue.

   Son x sur la ligne y vaut : xFirst + (y - firstLine) * slope.
   - xFirst est calculé (division 64 bits) sur sa première ligne ;
   - slope est la pente en 16.16 (pixels par ligne).
   Les deux divisions arrondissent vers le bas : le x calculé n'est jamais
   à droite du x exact. Quand un centre de pixel tombe exactement sur
   l'arête, il reste donc dedans pour une arête gauche (incluse) et dehors
   pour une arête droite (exclue), comme le veut la règle. Arrondir vers
   zéro, comme le fait la division du C, se trompe une fois sur deux.
   Ce x ne dépend que des deux sommets de l'arête et de la ligne : deux
   triangles qui partagent une arête trouvent exactement le même x, et la
   règle « gauche incluse, droite exclue » attribue chaque pixel à un seul
   d'entre eux. C'est pour cela qu'on ne recalcule pas x à partir d'une
   autre ligne de départ. */

typedef struct {
  int firstLine; /* première ligne couverte */
  int endLine;   /* ligne qui suit la dernière couverte */
  GrFixed xFirst;
  GrFixed slope;
} Edge;

/* Division arrondie vers le bas (b > 0) */
static int64_t floorDiv(int64_t a, int64_t b) {
  int64_t q = a / b;
  if (a % b < 0)
    q--;
  return q;
}

static void edgeInit(Edge *e, const GrVertex *top, const GrVertex *bottom) {
  e->firstLine = ceilFixed(top->y - GR_FIXED_HALF);
  e->endLine = ceilFixed(bottom->y - GR_FIXED_HALF);
  e->xFirst = top->x;
  e->slope = 0;

  if (e->firstLine >= e->endLine)
    return; /* aucune ligne : arête (presque) horizontale */

  const int64_t dx = (int64_t)bottom->x - top->x;
  const int64_t dy = (int64_t)bottom->y - top->y; /* > 0 */

  /* Distance du sommet au centre de la première ligne */
  const int64_t ty = (int64_t)e->firstLine * GR_FIXED_ONE + GR_FIXED_HALF - top->y;
  e->xFirst = (GrFixed)(top->x + floorDiv(ty * dx, dy));

  /* Une arête de moins d'un pixel de haut couvre au plus une ligne : sa
     pente ne sert pas, et peut dépasser 32 bits. On la borne. */
  int64_t slope = floorDiv(dx * GR_FIXED_ONE, dy);
  if (slope > (1 << 29))
    slope = 1 << 29;
  if (slope < -(1 << 29))
    slope = -(1 << 29);
  e->slope = (GrFixed)slope;
}

static GrFixed edgeX(const Edge *e, int y) {
  return (GrFixed)(e->xFirst + (int64_t)(y - e->firstLine) * e->slope);
}

/* Trace les lignes [yStart, yEnd[ de ce CPU entre deux arêtes */
static void drawSegment(const Rasterizer *r, int yStart, int yEnd,
                        const Edge *left, const Edge *right, GrColor_t color) {
  if (yStart < r->clipMinY)
    yStart = r->clipMinY;
  if (yEnd > r->clipMaxY)
    yEnd = r->clipMaxY;
  yStart = firstOwnLine(r, yStart);
  if (yStart >= yEnd)
    return;

  GrFixed xl = edgeX(left, yStart);
  GrFixed xr = edgeX(right, yStart);
  const GrFixed stepL = left->slope * 2; /* deux lignes : SLI */
  const GrFixed stepR = right->slope * 2;

  for (int y = yStart; y < yEnd; y += 2) {
    /* Pixels dont le centre (x + 0,5) est dans [xl, xr[ */
    int x0 = ceilFixed(xl - GR_FIXED_HALF);
    int x1 = ceilFixed(xr - GR_FIXED_HALF);
    if (x0 < r->clipMinX)
      x0 = r->clipMinX;
    if (x1 > r->clipMaxX)
      x1 = r->clipMaxX;
    if (x0 < x1)
      fillSpan(y, x0, x1, color);

    xl += stepL;
    xr += stepR;
  }
}

static void drawTriangle(const Rasterizer *r, const uint32_t *v) {
  GrVertex p[3];
  for (int i = 0; i < 3; i++) {
    p[i].x = (GrFixed)*v++;
    p[i].y = (GrFixed)*v++;
  }

  /* Tri par y croissant : p0 en haut, p2 en bas */
  GrVertex t;
  if (p[1].y < p[0].y) { t = p[0]; p[0] = p[1]; p[1] = t; }
  if (p[2].y < p[1].y) { t = p[1]; p[1] = p[2]; p[2] = t; }
  if (p[1].y < p[0].y) { t = p[0]; p[0] = p[1]; p[1] = t; }

  /* Produit vectoriel : de quel côté de la grande arête p0-p2 est p1 ?
     (y vers le bas) < 0 : à gauche ; 0 : triangle plat, rien à tracer. */
  const int64_t cross = ((int64_t)p[1].x - p[0].x) * ((int64_t)p[2].y - p[0].y) -
                        ((int64_t)p[2].x - p[0].x) * ((int64_t)p[1].y - p[0].y);
  if (cross == 0)
    return;

  Edge longEdge, upper, lower;
  edgeInit(&longEdge, &p[0], &p[2]);
  edgeInit(&upper, &p[0], &p[1]);
  edgeInit(&lower, &p[1], &p[2]);

  const GrColor_t color = r->sliDebug ? r->sliColor[r->parity] : r->color;

  if (cross < 0) { /* p1 à gauche : les arêtes courtes sont à gauche */
    drawSegment(r, upper.firstLine, upper.endLine, &upper, &longEdge, color);
    drawSegment(r, lower.firstLine, lower.endLine, &lower, &longEdge, color);
  } else {
    drawSegment(r, upper.firstLine, upper.endLine, &longEdge, &upper, color);
    drawSegment(r, lower.firstLine, lower.endLine, &longEdge, &lower, color);
  }
}

/* =========================================================================
   VDP
   ========================================================================= */

/* Table de lignes du buffer de dessin : l'image suit la table
   (screenHeight mots), une ligne tous les 160 mots. */
static void lineTableInit(int screenHeight) {
  const uint16_t first = (uint16_t)screenHeight;
  uint16_t *p = vdp_dram;

  for (int y = 0; y < screenHeight; y++)
    *p++ = (uint16_t)(first + y * (VDP_SCREEN_WIDTH / 2));
}

/* Attend le VBlank (VBLK passe à 1) */
static void waitForVBlank(void) {
  while ((vdp_read_fbcr() & FBCR_VBLK) == 0)
    if (!platform_running())
      return;
}

/* Attend la fin du VBlank (VBLK passe à 0) */
static void waitForDisplay(void) {
  while ((vdp_read_fbcr() & FBCR_VBLK) != 0)
    if (!platform_running())
      return;
}
