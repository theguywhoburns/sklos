#include <sysinfo.h>
#include <util.h>
bool GetBasicSystemInfo(SystemInfo* info, uint16_t bootDrive, void* partition) {
    info->boot_drive = bootDrive;
    info->partition = partition;
    return true;
}