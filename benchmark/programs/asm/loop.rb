Section :text

Li x1, 3000

Label :Loop
Addi x2, x2, 1
Addi x1, x1, -1
Beq x1, x0, :Done
J :Loop

Label :Done
Li x8, 60
Li x0, 42
syscall
