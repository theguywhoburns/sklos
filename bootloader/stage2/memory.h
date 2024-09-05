#pragma once

#include <util.h>

void* memcpy(void* dest, const void* src, uint32_t count);
void* memset(void* dest, uint8_t value, uint32_t count);
int memcmp(const void* dest, const void* src, uint32_t count);
void* memmove(void* dest, const void* src, uint32_t count);