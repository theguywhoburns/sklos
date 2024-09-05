#pragma once

#include <util.h>

typedef struct _DISK {
    uint8_t id;
    uint16_t cylinders;
    uint16_t sectors;
    uint8_t heads;
} packed DISK;

bool DISK_init(DISK* disk, uint8_t drive_number);
bool DISK_read(DISK* disk, uint32_t lba, uint32_t sectors, uint8_t* buffer_start);
void DISK_LBA2CHS(DISK* disk, uint32_t lba, uint16_t* cylinder, uint16_t* sector, uint16_t* head);
