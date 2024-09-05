#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define attr(attr) __attribute__((attr))
#define align(N) attr(aligned(N))
#define cdecl attr(cdecl)
#define packed attr(packed)

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#define char_toupper(c) (((c) >= 'a' && (c) <= 'z') ? ((c) - 'a' + 'A') : (c))
#define char_tolower(c) (((c) >= 'A' && (c) <= 'Z') ? ((c) - 'A' + 'a') : (c))