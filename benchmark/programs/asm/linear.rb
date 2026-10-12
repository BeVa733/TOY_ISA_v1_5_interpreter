Section :text

5000.times do
  Addi x1, x1, 1  # Not valid synax, just for benchmark
end

Li x8, 60
Li x0, 42
syscall
