class Instruction
  attr_reader :name, :operands, :pc, :line

  def initialize(name, operands, pc, line)
    @name = name
    @operands = operands
    @pc = pc
    @line = line
  end
end

class LabelInfo
  attr_reader :section, :offset

  def initialize(section, offset)
    @section = section
    @offset = offset
  end
end

class Register
  attr_reader :index

  def initialize(index)
    @index = index
  end
end

class Assembler
  attr_reader :instructions, :labels, :data

  INSTRUCTION_SIZE = 4
  SECTIONS = [:text, :data]
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
    @current_section = nil
    @opened_sections = {}
    @text_offset = 0
    @labels = {}
    @instructions = []
    @data = "".b
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
      require_section(:text, "Instruction #{name}")
      location = caller_locations(1, 1).first
      make_instruction(name, operands, location.lineno)
      @text_offset += INSTRUCTION_SIZE
    end
  end

  define_method(:Section) do |name|
    open_section(name)
  end

  define_method(:Label) do |name|
    unless name.is_a?(Symbol)
      raise "Label name must be a Symbol, got #{name.class}"
    end

    define_label(name)
  end

  define_method(:Bytes) do |value|
    require_section(:data, "Bytes")
    unless value.is_a?(String)
      raise "Bytes value must be a String, got #{value.class}"
    end

    @data << value.b
  end

  def make_instruction(name, operands, line)
    @instructions << Instruction.new(name, operands, @text_offset, line)
  end

  def open_section(name)
    unless SECTIONS.include?(name)
      raise "Unknown section #{name}"
    end
    if @opened_sections.key?(name)
      raise "Section #{name} is already defined"
    end

    @opened_sections[name] = true
    @current_section = name
  end

  def define_label(name)
    if @current_section.nil?
      raise "Label #{name} is defined outside of a section"
    end
    raise "Multiple definition of label #{name}" if @labels.key?(name)

    offset = @current_section == :text ? @text_offset : @data.bytesize
    @labels[name] = LabelInfo.new(@current_section, offset)
  end

  def require_section(expected, entity)
    return if @current_section == expected

    raise "#{entity} must be defined in .#{expected} section"
  end

  private :define_label, :open_section, :require_section
end
