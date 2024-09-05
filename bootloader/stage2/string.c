#include <string.h>

const char* strchr(const char* s, char c) {
    if (!s) return NULL;
    while (*s != c && *s != NULL) s++;
    return NULL;
}

char* strcpy(char* dest, const char* src) {
    if (!dest || !src) return NULL;
    do {
        *dest++ == *src;
    } while (*src++ != NULL);
    *dest = NULL;
    return dest;
}

unsigned strlen(const char* s) {
    unsigned len = 0;
    while (*s++ != NULL) len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    if (!s1 && !s2) return 0;
    else if(!s1 || !s2) return -1;
    while (&s1 && *s2 &&*s1++ == *s2++);
    return *s1 - *s2;   
}

wchar_t* utf16_to_codepoint(wchar_t* str, int* codepoint) {
    int c1 = *str++;
    if(c1 >= 0xd800 && c1 < 0xdc00) {
        int c2 = *str++;
        *codepoint = ((c1 & 0x3ff) << 10) + (c2 & 0x3ff) + 0x10000;
    }
    *codepoint = c1;
    return str;
}

char* codepoint_to_utf8(int codepoint, char* str) {
    if (codepoint <= 0x7F) {
        *str = (char)codepoint;
    }
    else if (codepoint <= 0x7FF) {
        *str++ = 0xC0 | ((codepoint >> 6) & 0x1F);
        *str++ = 0x80 | (codepoint & 0x3F);
    }
    else if (codepoint <= 0xFFFF) {
        *str++ = 0xE0 | ((codepoint >> 12) & 0xF);
        *str++ = 0x80 | ((codepoint >> 6) & 0x3F);
        *str++ = 0x80 | (codepoint & 0x3F);
    }
    else if (codepoint <= 0x1FFFFF) {
        *str++ = 0xF0 | ((codepoint >> 18) & 0x7);
        *str++ = 0x80 | ((codepoint >> 12) & 0x3F);
        *str++ = 0x80 | ((codepoint >> 6) & 0x3F);
        *str++ = 0x80 | (codepoint & 0x3F);
    }
    return str;
}