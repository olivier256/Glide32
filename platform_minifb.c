#include "platform.h"
#include "game.h"
#include "vdp.h"

#include <MiniFB.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define DEFAULT_SCREEN_HEIGHT 224
#define VBLANK_NS 2420000L

static struct mfb_window *_window = NULL;
static uint32_t *_display = NULL;
static int _width = 0;
static int _height = 0;

static pthread_t _slave_thread;
static void (*_slave_main)(void);

static pthread_t _vblank_thread;
static int _vblank_thread_started = 0;

static struct timespec _vblank_start;

/* ==== Point d'entrée : joue le rôle de la BootROM ===================== */

int main(int argc, char *argv[]) {
  int h = DEFAULT_SCREEN_HEIGHT;
  if (argc > 1) {
    h = atoi(argv[1]);
    if (h <= 0)
      return 1;
  }

  vdp_init(h);

  if (!platform_init(VDP_SCREEN_WIDTH, h))
    return 1;

  /* Lancer les deux CPU à leurs points d'entrée */
  if (platform_start_slave(slave_main) != 0) {
    platform_shutdown();
    return 1;
  }

  /* Étape 1 : le master reste sur le thread principal, car c'est lui qui
     appelle platform_wait_vsync(), donc MiniFB (Cocoa exige le thread
     principal). Il aura son propre thread à l'étape 2. */
  master_main();

  platform_join_slave();
  platform_shutdown();

  return 0;
}

/* ==== Threads =========================================================== */

static void *_slave_trampoline(void *arg) {
  (void)arg;
  _slave_main();
  return NULL;
}

int platform_start_slave(void (*slave_main)(void)) {
  _slave_main = slave_main;
  return pthread_create(&_slave_thread, NULL, _slave_trampoline, NULL);
}

int platform_join_slave(void) { return pthread_join(_slave_thread, NULL); }

/* Apple Silicon réordonne les écritures entre cœurs : barrière complète */
void platform_sync(void) { __sync_synchronize(); }

/* ==== Fenêtre =========================================================== */

int platform_init(int width, int height) {
  _width = width;
  _height = height;

  _window = mfb_open("VDP", width, height);

  if (_window == NULL)
    return 0;

  _display = malloc((size_t)width * height * sizeof(uint32_t));

  if (_display == NULL) {
    platform_shutdown();
    return 0;
  }

  return 1;
}

void platform_shutdown(void) {
  if (_vblank_thread_started) {
    pthread_join(_vblank_thread, NULL);
    _vblank_thread_started = 0;
  }

  free(_display);
  _display = NULL;

  if (_window != NULL) {
    mfb_close(_window);
    _window = NULL;
  }
}

/* ==== Synchro vidéo ===================================================== */

static void *vblank_timer(void *arg) {
  (void)arg;

  struct timespec duration = {0, VBLANK_NS};
  nanosleep(&duration, NULL);

  vdp_end_vblank();

  return NULL;
}

static int start_vblank_timer(void) {
  /* Récupérer les ressources du thread précédent */
  if (_vblank_thread_started) {
    pthread_join(_vblank_thread, NULL);
    _vblank_thread_started = 0;
  }

  /* Lancer le nouveau timer */
  if (pthread_create(&_vblank_thread, NULL, vblank_timer, NULL) != 0)
    return 0;

  _vblank_thread_started = 1;
  return 1;
}

static int present(void) {
  if (_window == NULL || _display == NULL)
    return 0;

  vdp_render(_display);

  mfb_update_state state = mfb_update(_window, _display);

  if (state == MFB_STATE_EXIT) {
    _window = NULL;
    return 0;
  }
  return state == MFB_STATE_OK;
}

int platform_wait_vsync(void) {
  if (_window == NULL)
    return 0;

  if (!mfb_wait_sync(_window))
    return 0;

  clock_gettime(CLOCK_MONOTONIC, &_vblank_start);
  vdp_begin_vblank();

  if (!start_vblank_timer()) {
    vdp_end_vblank();
    return 0;
  }

  return present();
}

double platform_vblank_elapsed_ms(void) {
  struct timespec now;

  clock_gettime(CLOCK_MONOTONIC, &now);

  return (now.tv_sec - _vblank_start.tv_sec) * 1000.0 +
         (now.tv_nsec - _vblank_start.tv_nsec) / 1000000.0;
}
