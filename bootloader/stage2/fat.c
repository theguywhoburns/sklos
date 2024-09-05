#include <fat.h>
#include <printf.h>
#include <util.h>
#include <string.h>
#include <memory.h>
#include <memdefs.h>
#include <x86.h>

#define SECTOR_SIZE             512
#define MAX_PATH_SIZE           256
#define MAX_FILE_HANDLES        10
#define ROOT_DIRECTORY_HANDLE   -1

typedef struct FATBootSector {
    uint8_t BootJumpInstruction[3]; // jmp and nop
    uint8_t OemIdentifier[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FatCount;
    uint16_t DirEntryCount;
    uint16_t TotalSectors;
    uint8_t MediaDescriptorType;
    uint16_t SectorsPerFat;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t LargeSectorCount;

    // extended boot record
    uint8_t DriveNumber;
    uint8_t _Reserved;
    uint8_t Signature;
    uint32_t VolumeId;          // serial number, value doesn't matter
    uint8_t VolumeLabel[11];    // 11 bytes, padded with spaces
    uint8_t SystemId[8];

    // ... we don't care about code ...

} packed FATBootSector;

typedef struct FATFileData {
    uint8_t Buffer[SECTOR_SIZE];
    FATFile Public;
    bool Opened;
    uint32_t FirstCluster;
    uint32_t CurrentCluster;
    uint32_t CurrentSectorInCluster;
} FATFileData;

typedef struct FATData {
    union {
        FATBootSector BootSector;
        uint8_t Buffer[SECTOR_SIZE];
    } BS;
    FATFileData RootDirectory;
    FATFileData OpenedFiles[MAX_FILE_HANDLES];
} FATData;

volatile static FATData* g_data;
volatile static uint8_t* g_FAT = NULL;
volatile static uint32_t g_DataSectLba;

bool FATReadBootSector(DISK* disk) {
    // Now it fails here lol
    // i'll link the debugger in a sec
    return DISK_read(disk, 0, 1, g_data->BS.Buffer);
}

bool FATReadFAT(DISK* disk) {
    return DISK_read(disk, g_data->BS.BootSector.ReservedSectors, g_data->BS.BootSector.SectorsPerFat, g_FAT);
}

bool FATInit(DISK* disk) {
    g_data = (FATData*)MEMORY_FAT_ADDR;
    printf("Reading FAT boot sector: ");
    if(!FATReadBootSector(disk)) {
        printf("FAILURE\n");
        printf("FAT: failed to read boot sector\n");
        return false;
    }
    printf("DONE\n");
    // read FAT
    g_FAT = (uint8_t)g_data + sizeof(FATData);
    uint32_t fatSize = g_data->BS.BootSector.SectorsPerFat * SECTOR_SIZE;
    printf("FAT address: 0x%p, size: %lu\n", g_FAT, fatSize);
    if(sizeof(FATData) + fatSize >= MEMORY_FAT_SIZE) {
        printf("FAT: not enough memory to read FAT! Required %lu, only have %u\n", sizeof(FATData) + fatSize, MEMORY_FAT_SIZE);
        return false;
    }
    printf("Reading FAT: ");
    if(!FATReadFAT(disk)) {
        printf("FAILURE\n");
        printf("FAT: failed to read FAT\n");
        return false;
    }
    printf("DONE\n");

    // open root dir
    uint32_t rootDirLba = g_data->BS.BootSector.ReservedSectors + g_data->BS.BootSector.FatCount * g_data->BS.BootSector.SectorsPerFat;
    uint32_t rootDirSize = g_data->BS.BootSector.DirEntryCount * sizeof(FATDirEntry);

    g_data->RootDirectory.Public.Handle = ROOT_DIRECTORY_HANDLE;
    g_data->RootDirectory.Public.IsDirectory = true;
    g_data->RootDirectory.Public.Position = 0;
    g_data->RootDirectory.Public.Size = sizeof(FATDirEntry) * g_data->BS.BootSector.DirEntryCount;
    g_data->RootDirectory.Opened = true;
    g_data->RootDirectory.FirstCluster = rootDirLba;
    g_data->RootDirectory.CurrentCluster = rootDirLba;
    g_data->RootDirectory.CurrentSectorInCluster = 0;

    if(!DISK_read(disk, rootDirLba, 1, g_data->RootDirectory.Buffer)) {
        printf("FAT: failed to read root directory\n");
        return false;
    }

    // calculate data section
    uint32_t rootDirSectors = (rootDirSize + g_data->BS.BootSector.BytesPerSector - 1) / g_data->BS.BootSector.BytesPerSector;
    g_DataSectLba = rootDirLba + rootDirSectors;

    // reset opened files
    for(int i = 0; i < MAX_FILE_HANDLES; i++) {
        g_data->OpenedFiles[i].Opened = false;
    }

    return true;
}

uint32_t FATClusterToLba(uint32_t cluster) {
    return g_DataSectLba + (cluster - 2) * g_data->BS.BootSector.SectorsPerCluster;
}

FATFile* FatOpenEntry(DISK* disk, FATDirEntry* entry) {
    // find empty handle
    int handle = -1;
    for(int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++) {
        if(!g_data->OpenedFiles[i].Opened) {
            handle = i;
        }
    }

    // out of handles
    if(handle < 0) {
        printf("FAT: out of file handles\n");
        return NULL;
    }

    // setup vars
    FATFileData* fd = &g_data->OpenedFiles[handle];
    fd->Public.Handle = handle;
    fd->Public.IsDirectory = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->Public.Position = 0;
    fd->Public.Size = entry->Size;
    fd->FirstCluster = entry->FirstClusterLow + ((uint32_t)entry->FirstClusterHigh << 16);
    fd->CurrentCluster = fd->FirstCluster;
    fd->CurrentSectorInCluster = 0;

    if(!DISK_read(disk, FATClusterToLba(fd->CurrentCluster), 1, fd->Buffer)) {
        printf("FAT: open entry failed - read error cluster=%u lba=%u\n", fd->CurrentCluster, FATClusterToLba(fd->CurrentCluster));
        for(int i = 0; i < 11; i++) {
            printf("%c", entry->Name[i]);
        }
        printf("\n");
        return NULL;
    }

    fd->Opened = true;
    return &fd->Public;
}

uint32_t FATNextCluster(uint32_t cluster) {
    uint32_t fatIndex = cluster * 3 / 2;

    if(cluster % 2 == 0) {
        return (*(uint16_t*)(g_FAT + fatIndex)) & 0x0FFF;
    } else {
        return (*(uint16_t*)(g_FAT + fatIndex)) >> 4;
    }
}

uint32_t FATRead(DISK* disk, FATFile* file, uint32_t count, void* buffer) {
    FATFileData* fd = (file->Handle == ROOT_DIRECTORY_HANDLE) 
        ? &g_data->RootDirectory 
        : &g_data->OpenedFiles[file->Handle];
    uint8_t* u8buffer = (uint8_t*)buffer;

    // don't read past end of file
    if(!fd->Public.IsDirectory && fd->Public.Size != 0) {
        count = min(count, fd->Public.Size - fd->Public.Position);
    }

    while(count > 0) {
        uint32_t buffer_left = SECTOR_SIZE - (fd->Public.Position % SECTOR_SIZE);
        uint32_t take = min(buffer_left, count);
        memcpy(u8buffer, fd->Buffer + (fd->Public.Position % SECTOR_SIZE), take);
        u8buffer += take;
        fd->Public.Position += take;
        count -= take;

        printf("FATRead: buffer_left=%u take=%u\n", buffer_left, take);
        // See if we need to read more data
        if(buffer_left == take) {
            // Special handling for root directory
            if (fd->Public.Handle == ROOT_DIRECTORY_HANDLE) {
                ++fd->CurrentCluster;

                // read next sector
                if (!DISK_read(disk, fd->CurrentCluster, 1, fd->Buffer)) {
                    printf("FAT: read error!\n");
                    break;
                }
            } else {
                // calculate next cluster & sector to read
                if (++fd->CurrentSectorInCluster >= g_data->BS.BootSector.SectorsPerCluster) {
                    fd->CurrentSectorInCluster = 0;
                    fd->CurrentCluster = FATNextCluster(fd->CurrentCluster);
                }

                if (fd->CurrentCluster >= 0xFF8) {
                    // Mark end of file
                    fd->Public.Size = fd->Public.Position;
                    break;
                }

                // read next sector
                if (!DISK_read(disk, FATClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster, 1, fd->Buffer)) {
                    printf("FAT: read error!\n");
                    break;
                }
            }
        }
    }
    return u8buffer - (uint8_t*)buffer;
}

bool FATReadEntry(DISK* disk, FATFile* file, FATDirEntry* entry) { 
    return FATRead(disk, file, sizeof(*entry), entry) == sizeof(*entry);
}

void FATClose(FATFile* file) {
    if (file->Handle == ROOT_DIRECTORY_HANDLE) {
        file->Position = 0;
        g_data->RootDirectory.CurrentCluster = g_data->RootDirectory.FirstCluster;
    } else {
        g_data->OpenedFiles[file->Handle].Opened = false;
    }
}

//void FATGetShortName(const char* name, char shortName[12]) {
//    // convert from name to FAT name
//    memset(shortName, ' ', sizeof(shortName));
//    shortName[11] = 0x0;
//
//    const char* ext = strchr(name, '.');
//    if(ext == NULL) ext = name + 11;
//    for(int i = 0; i < 8 && name[i] && name + i < ext; i++) {
//        shortName[i] = char_toupper(name[i]);
//    }
//    if(ext != name + 11) {
//        for (int i = 0; i < 3 && ext[i + 1]; i++) {
//            shortName[i + 8] = char_toupper(ext[i + 1]);
//        }
//    }
//}

bool FATFindFile(DISK* disk, FATFile* file, const char* name, FATDirEntry* entry) {
    char fatName[12];
    FATDirEntry _entry;

    // convert from name to fat name
    memset(fatName, ' ', sizeof(fatName));
    fatName[11] = '\0';

    const char* ext = strchr(name, '.');
    if (ext == NULL)
        ext = name + 11;

    for (int i = 0; i < 8 && name[i] && name + i < ext; i++)
        fatName[i] = char_toupper(name[i]);

    if (ext != name + 11)
    {
        for (int i = 0; i < 3 && ext[i + 1]; i++)
            fatName[i + 8] = char_toupper(ext[i + 1]);
    }

    while (FATReadEntry(disk, file, &_entry)) {
        if (memcmp(fatName, _entry.Name, 11) == 0) {
            *entry = _entry;
            return true;
        }        
    }
    
    return false;
}

FATFile* FATOpen(DISK* disk, const char* path) {
    char name[MAX_PATH_SIZE];
    // ignore leading slash
    if(path[0] == '/') path++;
    if(path[0] == '\0') return NULL;

    FATFile* current = &g_data->RootDirectory.Public;
    while (*path) {
        // extract next file name from path
        bool isLast = false;
        const char* delim = strchr(path, '/');
        if (delim != NULL) {
            memcpy(name, path, delim - path);
            name[delim - path + 1] = '\0';
            path = delim + 1;
        } else {
            unsigned len = strlen(path);
            memcpy(name, path, len);
            name[len + 1] = '\0';
            path += len;
            isLast = true;
        }

        FATDirEntry entry;
        if(!FATFindFile(disk, current, name, &entry)) {
            FATClose(current);
            printf("FAT: file not found: %s\n", name);
            return NULL;
        } else {
            FATClose(current);
            if(!isLast && entry.Attributes & FAT_ATTRIBUTE_DIRECTORY == 0) {
                printf("FATL %s is not a directory\n", name);
                return NULL;
            }
            // open new directory entry
            current = FatOpenEntry(disk, &entry);
        }
    }
    return current;   
}