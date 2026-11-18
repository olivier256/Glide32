#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

int platform_init(int width, int height);
int platform_wait_vsync(void);
void platform_shutdown(void);

#endif