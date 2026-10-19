# This program is assembled once into jit.elf for the jit execution test.

Section :text

Li x1, 10
Li x2, 20
Add x3, x1, x2
Addi x4, x3, -5
Or x5, x1, x2
Clz x6, x1
Ssat x7, x2, 4
Rori x9, x1, 1

Li x10, 0xD6
Li x11, 0xAC
Bext x12, x10, x11

Li x13, 128
St x3, [x13, 0]
Ld x14, [x13, 0]
Li x15, 4
St x4, [x13, 4]
Ld x16, [x13, x15]
Stp x1, x2, [x13, 8]

Li x17, 3
Label :Loop
Addi x17, x17, -1
Beq x17, x0, :Done
J :Loop

Label :Done
Li x8, 60
Li x0, 42
syscall
