#include "vdp.h"
#include "platform.h"
#include "sh2mem.h"

/* Mémoire */
static uint16_t vdp_dram0[VDP_DRAM_SIZE / 2];
static uint16_t vdp_dram1[VDP_DRAM_SIZE / 2];
uint16_t vdp_palette[VDP_PALETTE_SIZE];

uint16_t *vdp_dram = vdp_dram1;
static uint16_t *displayFrameBuffer = vdp_dram0;

static int _screenHeight = 224;

/* Registres */
static volatile uint16_t _bitmapModeRegister;
volatile uint16_t *const bitmapModeRegister = &_bitmapModeRegister;

/* représente la valeur effectivement appliquée */
static uint16_t _activeFS = 0;
/* conserve la dernière valeur écrite */
static uint16_t _requestedFS = 0;
static volatile int _inVBlank = 0;

/* Diagnostics */
static volatile unsigned long _vblanks = 0;
static volatile unsigned long _seen[PLATFORM_CPU_COUNT];
static unsigned long _lastSeenVBlank[PLATFORM_CPU_COUNT];

/* met à jour les deux pointeurs de mémoire */
static void applyFrameBuffer(void);

void vdp_init(int screenHeight) { _screenHeight = screenHeight; }

int vdp_screen_height(void) { return _screenHeight; }

/* renvoie la valeur active, pas nécessairement la valeur positionnée */
uint16_t vdp_read_fbcr(void) {
  uint16_t value = _activeFS ? FBCR_FS : 0;

  if (_inVBlank) {
    value |= FBCR_VBLK;

    /* Diagnostics : ce CPU a vu ce VBlank */
    int cpu = platform_cpu_id();
    unsigned long vblank = _vblanks;
    if (cpu >= 0 && _lastSeenVBlank[cpu] != vblank) {
      _lastSeenVBlank[cpu] = vblank;
      _seen[cpu]++;
    }
  }

  return value;
}

void vdp_write_fbcr(uint16_t value) {
  _requestedFS = value & FBCR_FS;

  if (_inVBlank || ((*bitmapModeRegister & 3) == 0))
    applyFrameBuffer();
}

/* Appelée par la synchro vidéo. L'ordre compte : les pointeurs
   doivent être échangés AVANT qu'un CPU puisse voir VBLK = 1. */
void vdp_begin_vblank(void) {
  applyFrameBuffer();
  _vblanks++;
  platform_sync();

  _inVBlank = 1;
}

void vdp_end_vblank(void) {
  platform_sync();
  _inVBlank = 0;
}

static void applyFrameBuffer(void) {
  _activeFS = _requestedFS;

  displayFrameBuffer = _activeFS ? vdp_dram1 : vdp_dram0;
  vdp_dram = _activeFS ? vdp_dram0 : vdp_dram1;
}

void vdp_get_stats(vdp_stats *stats) {
  stats->vblanks = _vblanks;
  for (int i = 0; i < PLATFORM_CPU_COUNT; i++)
    stats->seen[i] = _seen[i];
}

/* ---- Rendu ------------------------------------------------------------- */

static uint32_t get_palette_color(uint8_t index) {
  uint16_t color = vdp_palette[index];

  uint8_t r = color & 0x1F;
  uint8_t g = (color >> 5) & 0x1F;
  uint8_t b = (color >> 10) & 0x1F;

  r = (r << 3) | (r >> 2);
  g = (g << 3) | (g >> 2);
  b = (b << 3) | (b >> 2);

  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void vdp_render_line(int line, uint32_t *rgb) {
  switch (*bitmapModeRegister & 3) {
  case 1: { // PACKED_PIXEL_MODE
    /* Un pixel = un octet, lus dans l'ordre des adresses : le pixel x est
       l'octet (adresse de ligne * 2 + x), comme sur la vraie 32X. */
    /* La table de lignes est en mots gros-boutistes (convention sh2mem.h) */
    const uint8_t *pixels = (const uint8_t *)displayFrameBuffer +
                            read16(displayFrameBuffer + line) * 2;
    for (int col = 0; col < VDP_SCREEN_WIDTH; col++)
      rgb[col] = get_palette_color(pixels[col]);
    break;
  }
  default: // BLANK_MODE ; DIRECT_COLOR_MODE, RUN_LENGTH_MODE : à faire
    for (int col = 0; col < VDP_SCREEN_WIDTH; col++)
      rgb[col] = 0;
    break;
  }
}
