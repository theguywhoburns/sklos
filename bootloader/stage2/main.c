#include <stdint.h>
#include <printf.h>
#include <memdefs.h>
#include <textout.h>
#include <util.h>
#include <x86.h>
#include <fat.h>
#include <memory.h>
#include <sysinfo.h>

typedef void cdecl (*kernel_entry)(SystemInfo* info);

volatile static SystemInfo g_info = {0};
// We'll use it to load kernel from disk first, then we'll store the systmem info in it
const uint8_t* LoadBuffer = (uint8_t*)MEMORY_LOAD_KERNEL;
volatile static uint8_t* Kernel = (uint8_t*)MEMORY_KERNEL_ADDR;

void cdecl start32(uint16_t bootDrive, void* partition) {
    cls();
    disable_cursor();
    printf("SklOS bootloader stage 2\n");
    printf("Booting from drive %d, partition 0x%p\n", bootDrive, partition);   

    if(! isCPUIDSupported()) {
        printf("CPUID is not supported, halting...\n");
        return;
    }
    printf("CPUID is supported...\n");
    if(!isLongModeSupported()) {
        printf("Long mode is not supported, halting...\n");
        return;
    }
    printf("Long mode is supported...\n");
    

    printf("Initializing disk and FAT...\n");
    DISK disk;
    DISK_init(&disk, g_info.boot_drive);
    printf("Disk initialized...\n");
    if(!FATInit(&disk)) {
        printf("FAT init failed\n");
        return;
    }
    printf("FAT initialized...\n");
    printf("Creating kernel file handle...\n");
    FATFile *kernel_file = FATOpen(&disk, "/kernel.bin");
    if(!kernel_file) { puts("Kernel file not found\n"); return; }

    printf("Reading kernel from disk...\n");
    uint32_t read;
    uint8_t* kernelBuffer = Kernel;
    while ((read = FATRead(&disk, kernel_file, MEMORY_LOAD_SIZE, LoadBuffer)) > 0) { 
        memcpy(kernelBuffer, LoadBuffer, read);
        kernelBuffer += read;
    }

    FATClose(kernel_file);
    printf("Kernel loaded... Getting basic system info to send to kernel buffer\n");
    if(!GetBasicSystemInfo(&g_info, bootDrive, partition)) {
        printf("Failed to get system info, halting...\n");
        return;
    }
    memcpy(&g_info, LoadBuffer, sizeof(SystemInfo));
    
    kernel_entry kernel = (kernel_entry)Kernel;
    kernel(&g_info);        
}    