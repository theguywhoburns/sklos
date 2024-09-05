#pragma once
#include <stdint.h>
#include <util.h>

typedef struct SystemInfo {
    uint16_t boot_drive;
    void* partition;
    union {
        char data;
        struct {
            char reserved : 8;
        };
    };
} packed SystemInfo;

bool GetBasicSystemInfo(SystemInfo* info, uint16_t bootDrive, void* partition);