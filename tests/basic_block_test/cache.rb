# This program is assembled once into cache.bin for the block cache test.

Li x1, 5
Addi x2, x1, 7
J :AfterJump
Li x3, 9

Label :AfterJump

Li x1, 5
Beq x0, x0, :AfterBranch
Li x3, 9

Label :AfterBranch

Li x1, 5
syscall
Li x3, 9
