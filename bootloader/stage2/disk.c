#include <printf.h>
#include <textout.h>
#include <x86.h>
#include <disk.h>

bool DISK_init(DISK* disk, uint8_t drive_number) {
    uint8_t driveType;
    uint16_t cylinders, sectors, heads;
    if(!get_drive_parameters(drive_number, &driveType, &cylinders, &sectors, &heads)) 
        return false;
    
    disk->id = drive_number;
    disk->cylinders = cylinders;
    disk->sectors = sectors;
    disk->heads = heads;
    return true;
}

void DISK_LBA2CHS(DISK* disk, uint32_t lba, uint16_t* cylinder, uint16_t* sector, uint16_t* head)  {
    *sector = lba % disk->sectors + 1;
    *cylinder = (lba / disk->sectors) / disk->heads;
    *head = (lba / disk->sectors) % disk->heads;
}

bool DISK_read(DISK* disk, uint32_t lba, uint32_t sectors, uint8_t* buffer_start) {
    uint16_t cylinder, sector, head;
    DISK_LBA2CHS(disk, lba, &cylinder, &sector, &head);
    for(int i = 0; i < 3; i++) {
        if(disk_read(disk->id, cylinder, sector, head, sectors, buffer_start)) {
            BochsBreak();
            return true;
        }
        
        
        disk_reset(disk->id);
    }
    return false;
}

