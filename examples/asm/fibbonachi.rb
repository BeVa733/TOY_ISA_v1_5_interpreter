
		Li   x1, 9 # fibbonachi number
		Li   x2, 2 # will be x1 when we get required numer

		Li   x0, 1 # result
		Li	 x3, 1 # fib before value

Label :Loop

		Addi x4, x0, 0   # save second value
		Add  x0, x0, x3  # calc current value
		Addi x3, x4, 0   # save last value
		Addi x1, x1, -1

		Beq  x1, x2, :Exit
		J    :Loop 

Label :Exit

		Li   x8, 60
		syscall



