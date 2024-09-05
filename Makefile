rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2)) $(wildcard $1$2)

# Compilers and linkers for both 32-bit and 64-bit
ASM=nasm

GCC_x86=i686-elf-gcc
LD_x86=i686-elf-ld
GCC_x64=x86_64-elf-gcc
LD_x64=x86_64-elf-ld

# Emulator
qemu=qemu-system-x86_64

# Wildcard files
bootloader_stage1_asm_files := $(call rwildcard,bootloader/stage1/,*.asm)
bootloader_stage2_asm_files := $(call rwildcard,bootloader/stage2/,*.asm)
bootloader_stage2_c_files   := $(call rwildcard,bootloader/stage2/,*.c)

# Object files
bootloader_stage2_objects := $(patsubst bootloader/stage2/%.asm, obj/bootloader/stage2/%.o, $(bootloader_stage2_asm_files))
bootloader_stage2_objects += $(patsubst bootloader/stage2/%.c,   obj/bootloader/stage2/%.o, $(bootloader_stage2_c_files))

# Compilation/Linking arguments
bootloader_stage2_gcc_args = -Ibootloader/stage2 -std=c99 -g -ffreestanding -nostdlib -c
bootloader_stage2_ASM_args = -f elf -g
bootloader_stage2_LD_args = -T bootloader/stage2/linker.ld -g -nostdlib -L/usr/lib/gcc/i686-elf/11.2.0/ -lgcc
bootloader_stage1_ASM_args = -f bin -g

# Create the OS floppy image
build/os_floppy.img: build/bootloader/stage1/bootloader_stage1.bin build/bootloader/stage2/bootloader_stage2.bin build/bootloader/stage2/bootloader_stage2.sym
	@dd if=/dev/zero of=$@ bs=512 count=2880 >/dev/null
	@mkfs.fat -F 12 -n "SKLOS" $@ >/dev/null
	@dd if=build/bootloader/stage1/bootloader_stage1.bin of=$@ conv=notrunc >/dev/null
	@mcopy -i $@ build/bootloader/stage2/bootloader_stage2.bin "::stage2.bin"
	@echo "--> Created: " $@

# Stage1 bootloader compilation
build/bootloader/stage1/bootloader_stage1.bin: $(bootloader_stage1_asm_files)
	@mkdir -p $(@D)
	$(ASM) $(bootloader_stage1_ASM_args) -o $@ $<

# Stage2 bootloader binary and symbols
build/bootloader/stage2/bootloader_stage2.bin: build/bootloader/stage2/bootloader_stage2.elf 
	@mkdir -p $(@D)
	objcopy -O binary $< $@

build/bootloader/stage2/bootloader_stage2.sym: build/bootloader/stage2/bootloader_stage2.elf 
	@mkdir -p $(@D)
	objcopy --only-keep-debug $< $@

# Stage2 bootloader ELF build
build/bootloader/stage2/bootloader_stage2.elf: $(bootloader_stage2_objects) bootloader/stage2/linker.ld	
	@mkdir -p $(@D)
	@echo $(bootloader_stage2_objects)
	$(LD_x86) $(bootloader_stage2_LD_args) -o $@ $(bootloader_stage2_objects)

# Object files generation for Stage2
obj/bootloader/stage2/%.o: bootloader/stage2/%.asm
	@mkdir -p $(@D)
	$(ASM) -f elf -g -o $@ $<

obj/bootloader/stage2/%.o: bootloader/stage2/%.c
	@mkdir -p $(@D)
	$(GCC_x86) $(bootloader_stage2_gcc_args) -o $@ $<

# Cleaning up generated files
clean: clean_bootloader

clean_bootloader:
	rm -rf obj build debug.log bx_enh_dbg.ini

# Rebuild everything
rebuild: clean all

# Debugging with Bochs
debug: build/os_floppy.img
	qemu-system-x86_64 -m 256M -hda build/os_floppy.img -serial file:serial.log -s -S

# Running with Bochs
run: all 
	bochs -qf .run_bochsrc -dbglog debug.log