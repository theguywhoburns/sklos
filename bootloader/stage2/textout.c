#include <textout.h>
#include <x86.h>
volatile char* buffer = (char*)0xb8000;
#define CMD_WIDTH 80
#define CMD_HEIGHT 25

volatile int x = 0;
volatile int y = 0;
volatile bool cursor_disabled = false;
void cls() {
    for(int i = 0; i < CMD_WIDTH * CMD_HEIGHT; i++) {
        buffer[i * 2] = ' ';
        buffer[i * 2 + 1] = 0x07;
    }
    x = 0;
    y = 0;
}

void putc(char c) {
    BochsConsolePrintChar(c);
    if (c == '\n') {
        x = 0;
        y++;
    } else if (c == '\b') {
        if (x > 0) {
            x--;
        }
    } else {
        buffer[(y * CMD_WIDTH + x) * 2] = c;
        x++;
    }

    if (x >= CMD_WIDTH) {
        x = 0;
        y++;
    }
    if (y >= CMD_HEIGHT) {
        //TODO: shift screen up by one line
        for(int i = 0; i < CMD_HEIGHT - 1; i++) {
            for(int j = 0; j < CMD_WIDTH; j++) {
                buffer[(i * CMD_WIDTH + j) * 2] = buffer[((i + 1) * CMD_WIDTH + j) * 2];
            }
        }
        y = CMD_HEIGHT - 1;
    }

    set_cursor(x, y);
}

void puts(char* s) {
    for(int i = 0; s[i] != 0; i++) {
        putc(s[i]);
    }
}

void set_cursor(int x, int y) {
    x = x % CMD_WIDTH;
    y = y % CMD_HEIGHT;
    if(cursor_disabled) return;
    uint16_t pos = y * CMD_WIDTH + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF)); // idk what i'm doing
}

void enable_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 0); // testing

    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | (uint8_t)(CMD_HEIGHT * CMD_WIDTH)); // May not work  

    cursor_disabled = false;
}

void disable_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
    cursor_disabled = true;
}