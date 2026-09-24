class Instruction
  attr_reader :name, :operands, :pc, :line

  def initialize(name, operands, pc, line)
    @name = name
    @operands = operands
    @pc = pc
    @line = line
  end
end

class Register
  attr_reader :index

  def initialize(index)
    @index = index
  end
end

class Assembler
  attr_reader :instructions, :labels

  INSTRUCTION_SIZE = 4
  INSTRUCTIONS = [
    :Add,
    :Addi,
    :Or,
    :Ld,
    :St,
    :Beq,
    :J,
    :Clz,
    :Ssat,
    :syscall,
    :Bext,
    :Li,
    :Rori,
    :Stp
  ]

  def initialize
    @pc = 0
    @labels = {}
    @instructions = []
  end

  def prog(&block)
    instance_eval(&block)
  end

  # Define registers
  32.times do |index|
    define_method("x#{index}") do
      Register.new(index)
    end
  end

  # Define instr methods
  INSTRUCTIONS.each do |name|
    define_method(name) do |*operands|
      location = caller_locations(1, 1).first
      make_instruction(name, operands, location.lineno)
      @pc += INSTRUCTION_SIZE
    end
  end

  define_method(:Label) do |name|
    unless name.is_a?(Symbol)
      raise "Label name must be a Symbol, got #{name.class}"
    end

    define_label(name)
  end

  def make_instruction(name, operands, line)
    @instructions << Instruction.new(name, operands, @pc, line)
  end

  def define_label(name)
    raise "Multiple definition of label #{name}" if @labels.key?(name)

    @labels[name] = @pc
  end

  private :define_label
end
