require_relative "assembler"

class Encoder
  def initialize(labels)
    @labels = labels
  end

  def encode(instruction)
    public_send("encode_#{instruction.name.downcase}", instruction)
  end

  def encode_st(instruction)
    require_operand_count(instruction, 2)
    rt, address = instruction.operands
    rt_index = register_index(rt, instruction.line)
    base_index, immediate = memory_address(address, 14, instruction.line)

    (0x20 << 26) | (base_index << 21) | (rt_index << 16) | immediate
  end

  def encode_addi(instruction)
    require_operand_count(instruction, 3)
    rt, rs, immediate = instruction.operands
    rt_index = register_index(rt, instruction.line)
    rs_index = register_index(rs, instruction.line)
    encoded_immediate = signed_immediate(immediate, 16, instruction.line)

    (0x17 << 26) | (rs_index << 21) | (rt_index << 16) | encoded_immediate
  end

  def encode_or(instruction)
    require_operand_count(instruction, 3)
    rd, rs, rt = instruction.operands
    rd_index = register_index(rd, instruction.line)
    rs_index = register_index(rs, instruction.line)
    rt_index = register_index(rt, instruction.line)

    (rs_index << 21) | (rt_index << 16) | (rd_index << 11) | 0x1A
  end

  def encode_ld(instruction)
    require_operand_count(instruction, 2)
    rt, address = instruction.operands
    rt_index = register_index(rt, instruction.line)
    required(address, Array, instruction.line)
    unless address.length == 2
      raise "Memory address must contain base and offset at line " \
            "#{instruction.line}"
    end

    base, offset = address
    base_index = register_index(base, instruction.line)

    if offset.is_a?(Integer)
      immediate = signed_immediate(offset, 14, instruction.line)
      return (0x33 << 26) | (base_index << 21) | (rt_index << 16) | immediate
    end

    if offset.is_a?(Register)
      offset_index = register_index(offset, instruction.line)
      return (0x03 << 26) | (base_index << 21) | (rt_index << 16) | (0x03 << 14) | offset_index
    end

    raise "LD offset must be Integer or Register at line #{instruction.line}"
  end

  def encode_j(instruction)
    require_operand_count(instruction, 1)
    target = label_address(instruction.operands[0], instruction.line)
    require_alignment(target, instruction.line)
    instruction_index = unsigned_immediate(target >> 2, 26, instruction.line)

    (0x3F << 26) | instruction_index
  end

  def encode_beq(instruction)
    require_operand_count(instruction, 3)
    rs, rt, target = instruction.operands
    rs_index = register_index(rs, instruction.line)
    rt_index = register_index(rt, instruction.line)
    offset = branch_offset(target, instruction)

    (0x10 << 26) | (rs_index << 21) | (rt_index << 16) | offset
  end

  def encode_clz(instruction)
    require_operand_count(instruction, 2)
    rd, rs = instruction.operands
    rd_index = register_index(rd, instruction.line)
    rs_index = register_index(rs, instruction.line)

    (rd_index << 21) | (rs_index << 16) | 0x32
  end

  def encode_ssat(instruction)
    require_operand_count(instruction, 3)
    rd, rs, immediate = instruction.operands
    rd_index = register_index(rd, instruction.line)
    rs_index = register_index(rs, instruction.line)
    encoded_immediate = unsigned_immediate(immediate, 5, instruction.line, 1)

    (0x08 << 26) | (rd_index << 21) | (rs_index << 16) | (encoded_immediate << 11)
  end

  def encode_add(instruction)
    require_operand_count(instruction, 3)
    rd, rs, rt = instruction.operands
    rd_index = register_index(rd, instruction.line)
    rs_index = register_index(rs, instruction.line)
    rt_index = register_index(rt, instruction.line)

    (rs_index << 21) | (rt_index << 16) | (rd_index << 11) | 0x0C
  end

  def encode_syscall(instruction)
    require_operand_count(instruction, 0)

    0x1E
  end

  def encode_bext(instruction)
    require_operand_count(instruction, 3)
    rd, rs1, rs2 = instruction.operands
    rd_index = register_index(rd, instruction.line)
    rs1_index = register_index(rs1, instruction.line)
    rs2_index = register_index(rs2, instruction.line)

    (rd_index << 21) | (rs1_index << 16) | (rs2_index << 11) | 0x34
  end

  def encode_li(instruction)
    require_operand_count(instruction, 2)
    rt, immediate = instruction.operands
    rt_index = register_index(rt, instruction.line)
    encoded_immediate = signed_immediate(immediate, 16, instruction.line)

    (0x0B << 26) | (rt_index << 16) | encoded_immediate
  end

  def encode_rori(instruction)
    require_operand_count(instruction, 3)
    rd, rs, immediate = instruction.operands
    rd_index = register_index(rd, instruction.line)
    rs_index = register_index(rs, instruction.line)
    encoded_immediate = unsigned_immediate(immediate, 5, instruction.line)

    (0x1D << 26) | (rd_index << 21) | (rs_index << 16) | (encoded_immediate << 11)
  end

  def encode_stp(instruction)
    require_operand_count(instruction, 3)
    rt1, rt2, address = instruction.operands
    rt1_index = register_index(rt1, instruction.line)
    rt2_index = register_index(rt2, instruction.line)
    base_index, offset = memory_address(address, 11, instruction.line)

    (0x15 << 26) | (base_index << 21) | (rt1_index << 16) | (rt2_index << 11) | offset
  end

  private

  def required(value, type, line)
    return if value.is_a?(type)

    raise "Incorrect operand type at line #{line}: expected #{type}, " \
          "got #{value.class}"
  end

  def require_operand_count(instruction, count)
    return if instruction.operands.length == count

    raise "Incorrect operand count for #{instruction.name} at line " \
          "#{instruction.line}: expected #{count}, " \
          "got #{instruction.operands.length}"
  end

  def register_index(register, line)
    required(register, Register, line)

    unless (0...32).cover?(register.index)
      raise "Register index must be in range 0..31 at line #{line}"
    end

    register.index
  end

  def signed_immediate(immediate, width, line)
    required(immediate, Integer, line)

    minimum = -(2**(width - 1))
    maximum = 2**(width - 1) - 1
    unless (minimum..maximum).cover?(immediate)
      raise "Immediate must fit in signed #{width} bits at line #{line}"
    end

    immediate & (2**width - 1)
  end

  def unsigned_immediate(immediate, width, line, minimum = 0)
    required(immediate, Integer, line)

    maximum = 2**width - 1
    unless (minimum..maximum).cover?(immediate)
      raise "Immediate must be in range #{minimum}..#{maximum} at line #{line}"
    end

    immediate
  end

  def memory_address(address, immediate_width, line)
    required(address, Array, line)
    unless address.length == 2
      raise "Memory address must contain base and offset at line #{line}"
    end

    base, immediate = address
    [register_index(base, line), signed_immediate(immediate, immediate_width, line)]
  end

  def label_address(label, line)
    required(label, Symbol, line)
    return @labels[label] if @labels.key?(label)

    raise "Unknown label #{label} at line #{line}"
  end

  def require_alignment(address, line)
    return if address % 4 == 0

    raise "Jump target must be aligned at line #{line}"
  end

  def branch_offset(target, instruction)
    return signed_immediate(target, 16, instruction.line) if target.is_a?(Integer)

    target_address = label_address(target, instruction.line)
    byte_offset = target_address - instruction.pc
    require_alignment(byte_offset, instruction.line)
    signed_immediate(byte_offset / 4, 16, instruction.line)
  end
end
