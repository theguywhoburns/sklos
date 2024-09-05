#pragma once

#include <util.h>

//outputs a character to the debug console
#define BochsConsolePrintChar(c) outb(0xe9, c)
//stops simulation and breaks into the debug console
#define BochsBreak() outw(0x8A00,0x8A00); outw(0x8A00,0x08AE0);

void cdecl outb(uint16_t port, uint8_t value);
uint8_t cdecl inb(uint16_t port);

void cdecl outw(uint16_t port, uint16_t value);
uint16_t cdecl inw(uint16_t port);

void cdecl outd(uint16_t port, uint32_t value);
uint32_t cdecl ind(uint16_t port);

bool cdecl get_drive_parameters(uint8_t drive, uint8_t* driveTypeOut, uint16_t* cylindersOut, uint16_t* sectorsOut, uint16_t* headsOut);
bool cdecl disk_reset(uint8_t drive);
bool cdecl disk_read(uint8_t drive, uint16_t cylinder, uint16_t sector, uint16_t head, uint8_t count, void* lowerDataOut);
bool cdecl isCPUIDSupported();
bool cdecl isLongModeSupported();