#include <memory.h>

void* memcpy(void* dest, const void* src, uint32_t count) {
    for(uint32_t i = 0; i < count; i++) 
        ((uint8_t*)dest)[i] = ((uint8_t*)src)[i];
}

void* memset(void* dest, uint8_t value, uint32_t count) {
    for(uint32_t i = 0; i < count; i++) 
        ((uint8_t*)dest)[i] = value;
}

int memcmp(const void* dest, const void* src, uint32_t count) {
    for(uint32_t i = 0; i < count; i++) 
        if(((uint8_t*)dest)[i] != ((uint8_t*)src)[i])
            return 1;
    return 0;
}   

void* memmove(void* dest, const void* src, uint32_t count) {
    // Copy from end to start
    // So we don't overwrite data we want to copy
    for(uint32_t i = count; i > 0; i--) {
        ((uint8_t*)dest)[i - 1] = ((uint8_t*)src)[i - 1];
    }
    return dest;
}
