# This tests was assembled with no-elf translator for 
# test decoder, this script not used for testing.

St x2, [x1, -4]
Addi x2, x1, -7
Or x3, x1, x2
Ld x4, [x1, x3]
J :Target
Beq x1, x2, :Target
Clz x3, x1
Ssat x2, x1, 8
Ld x3, [x1, -4]
Add x3, x1, x2
syscall
Bext x3, x1, x2
Li x2, -1
Rori x2, x1, 7
Stp x2, x3, [x1, -4]

Label :Target
