bits 32
section .text

global outb
global inb
global outw
global inw
global outd
global ind

; b - byte char
; w - word = 2 bytes short
; d - double word = 4 bytes int

outb:
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

inb:
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret

outw:
    mov dx, [esp + 4]
    mov ax, [esp + 8]
    out dx, ax
    ret

inw:
    mov dx, [esp + 4]
    xor eax, eax
    in ax, dx
    ret

outd:
    mov dx, [esp + 4]
    mov eax, [esp + 8]
    out dx, eax
    ret

ind:
    mov dx, [esp + 4]
    xor eax, eax
    in eax, dx
    ret

%macro x86_EnterRealMode 0
    [bits 32]
    jmp word 18h:.pmode16         ; 1 - jump to 16-bit protected mode segment

.pmode16:
    [bits 16]
    ; 2 - disable protected mode bit in cr0
    mov eax, cr0
    and al, ~1
    mov cr0, eax

    ; 3 - jump to real mode
    jmp word 00h:.rmode

.rmode:
    ; 4 - setup segments
    mov ax, 0
    mov ds, ax
    mov ss, ax

    ; 5 - enable interrupts
    sti

%endmacro


%macro x86_EnterProtectedMode 0
    cli

    ; 4 - set protection enable flag in CR0
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; 5 - far jump into protected mode
    jmp dword 08h:.pmode


.pmode:
    ; we are now in protected mode!
    [bits 32]
    
    ; 6 - setup segment registers
    mov ax, 0x10
    mov ds, ax
    mov ss, ax

%endmacro

; Convert linear address to segment:offset address
; Args:
;    1 - linear address
;    2 - (out) target segment (e.g. es)
;    3 - target 32-bit register to use (e.g. eax)
;    4 - target lower 16-bit half of #3 (e.g. ax)

%macro LinearToSegOffset 4

    mov %3, %1      ; linear address to eax
    shr %3, 4
    mov %2, %4
    mov %3, %1      ; linear address to eax
    and %3, 0xf

%endmacro

global get_drive_parameters
get_drive_parameters:
    [bits 32]
    ; make new call frame
    push ebp    ; save old ebp
    mov ebp, esp; new ebp

    x86_EnterRealMode

    [bits 16]

    ; save regs
    push es
    push bx
    push esi
    push di

     ; call int13h
    mov dl, [bp + 8]    ; dl - disk drive
    mov ah, 08h
    mov di, 0           ; es:di - 0000:0000
    mov es, di
    stc
    int 13h

    ; out params
    mov eax, 1
    sbb eax, 0

    ; drive type from bl
    LinearToSegOffset [bp + 12], es, esi, si
    mov [es:si], bl

    ; cylinders
    mov bl, ch          ; cylinders - lower bits in ch
    mov bh, cl          ; cylinders - upper bits in cl (6-7)
    shr bh, 6
    inc bx

    LinearToSegOffset [bp + 16], es, esi, si
    mov [es:si], bx

    ; sectors
    xor ch, ch          ; sectors - lower 5 bits in cl
    and cl, 3Fh
    
    LinearToSegOffset [bp + 20], es, esi, si
    mov [es:si], cx

    ; heads
    mov cl, dh          ; heads - dh
    inc cx

    LinearToSegOffset [bp + 24], es, esi, si
    mov [es:si], cx

    ; restore regs
    pop di
    pop esi
    pop bx
    pop es

    ; return

    push eax

    x86_EnterProtectedMode
    
    [bits 32]

    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global disk_reset
disk_reset:
    [bits 32]
    ; make new call frame
    push ebp    ; save old ebp
    mov ebp, esp; new ebp

    x86_EnterRealMode

    mov ah, 0
    mov dl, [bp + 8]    ; dl - drive
    stc
    int 13h

    mov eax, 1
    sbb eax, 0           ; 1 on success, 0 on fail   

    push eax

    x86_EnterProtectedMode

    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global disk_read 
disk_read:
    [bits 32]
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp          ; initialize new call frame

    x86_EnterRealMode

    ; save modified regs
    push ebx
    push es

    ; setup args
    mov dl, [bp + 8]    ; dl - drive

    mov ch, [bp + 12]    ; ch - cylinder (lower 8 bits)
    mov cl, [bp + 13]    ; cl - cylinder to bits 6-7
    shl cl, 6
    
    mov al, [bp + 16]    ; cl - sector to bits 0-5
    and al, 3Fh
    or cl, al

    mov dh, [bp + 20]   ; dh - head

    mov al, [bp + 24]   ; al - count

    LinearToSegOffset [bp + 28], es, ebx, bx

    ; call int13h
    mov ah, 02h
    stc
    int 13h

    ; set return value
    mov eax, 1
    sbb eax, 0           ; 1 on success, 0 on fail   

    ; restore regs
    pop es
    pop ebx

    push eax

    x86_EnterProtectedMode

    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

; bool cdecl check_CPUID_support();
global isCPUIDSupported
isCPUIDSupported:   
    [bits 32]    
    push ebx         ; save ebx for the caller
	pushfd           ; push eflags on the stack
	pop eax          ; pop them into eax
	mov ebx, eax     ; save to ebx for restoring afterwards
	xor eax, 200000h ; toggle bit 21
	push eax         ; push the toggled eflags
	popfd            ; pop them back into eflags
	pushfd           ; push eflags
	pop eax          ; pop them back into eax
	cmp eax, ebx     ; see if bit 21 was reset
	jz .not_supported
	
	mov eax, 1
	jmp .exit
	
.not_supported:
	xor eax, eax;

.exit:
	pop ebx
	ret

global isLongModeSupported
isLongModeSupported:
    [bits 32]
    mov eax, 0x80000000    ; Set the A-register to 0x80000000.
    cpuid                  ; CPU identification.
    cmp eax, 0x80000001    ; Compare the A-register with 0x80000001.
    jb .NoLongMode         ; It is less, there is no long mode.

    mov eax, 0x80000001    ; Set the A-register to 0x80000001.
    cpuid                  ; CPU identification.
    test edx, 1 << 29      ; Test if the LM-bit, which is bit 29, is set in the D-register.
    jz .NoLongMode         ; They aren't, there is no long mode.

    mov eax, 1
    jmp .exit
.NoLongMode
    xor eax, eax

.exit:
    ret
