set debug-file-directory .
set sysroot .
symbol-file build/bootloader/stage2/bootloader_stage2.sym
break start32
c