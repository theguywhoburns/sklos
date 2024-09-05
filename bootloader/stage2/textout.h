#pragma once

#include <stdint.h>

void putc(char c);
void puts(char* s);
void cls();
void set_cursor(int x, int y);
void enable_cursor();
void disable_cursor();