
#ifndef MAIN_H
#define MAIN_H
#pragma once

#include <stdint.h>

void setPixel(int x, int y, uint8_t c);
void lineTableInit(const int screenHeight, const int BITMAP_OFFSET);
#endif