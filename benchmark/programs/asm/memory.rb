Section :text

Li x1, 2000
Li x2, 3000
Li x3, 1

Label :Loop
St x3, [x1, 0]
Ld x4, [x1, 0]
Addi x3, x3, 1
Addi x2, x2, -1
Beq x2, x0, :Done
J :Loop

Label :Done
Li x8, 60
Li x0, 42
syscall
