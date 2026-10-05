# Assembler test for checking correctness of encoding instructions 
# and reaction to incorrect programm.

require_relative "../assembler/modules/encoder"

def assemble(&program)
  assembler = Assembler.new
  assembler.Section(:text)
  assembler.instance_eval(&program)
  encoder = Encoder.new(assembler.labels)
  assembler.instructions.map { |instruction| encoder.encode(instruction) }
end

def check_program(name, expected, &program)
  actual = assemble(&program)
  return if expected == actual

  raise "#{name}: expected #{expected.inspect}, got #{actual.inspect}"
end

def check_raises(message, &program)
  begin
    assemble(&program)
  rescue RuntimeError
    return
  end

  raise message
end

check_program("ST",           [0x80223FFC]) { St x2, [x1, -4] }
check_program("ADDI",         [0x5C22FFF9]) { Addi x2, x1, -7 }
check_program("OR",           [0x0022181A]) { Or x3, x1, x2 }
check_program("LD register",  [0x0C24C003]) { Ld x4, [x1, x3] }
check_program("J",            [0xFC000001]) {
  J :Target
  Label :Target
}
check_program("BEQ",          [0x40220001]) {
  Beq x1, x2, :Target
  Label :Target
}
check_program("CLZ",          [0x00610032]) { Clz x3, x1 }
check_program("SSAT",         [0x20414000]) { Ssat x2, x1, 8 }
check_program("LD immediate", [0xCC233FFC]) { Ld x3, [x1, -4] }
check_program("ADD",          [0x0022180C]) { Add x3, x1, x2 }
check_program("SYSCALL",      [0x0000001E]) { syscall }
check_program("BEXT",         [0x00611034]) { Bext x3, x1, x2 }
check_program("LI",           [0x2C02FFFF]) { Li x2, -1 }
check_program("RORI",         [0x74413800]) { Rori x2, x1, 7 }
check_program("STP",          [0x54221FFC]) { Stp x2, x3, [x1, -4] }


check_raises("Assembler accepted a non-Symbol label") { Label "Loop" }
check_raises("Assembler accepted a duplicate label") {
  Label :Loop
  Label :Loop
}
check_raises("Encoder accepted an unknown label") { J :Unknown }
check_raises("Encoder accepted an out-of-range immediate") { Li x1, 32_768 }
check_raises("Encoder accepted an incorrect operand count") { Add x1, x2 }

puts "All assembler tests passed"
