#include "../include/toy/bitutils.hpp"
#include "../include/toy/decoder.hpp"
#include "test_utils.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void checkInstruction(std::ifstream &File, TIDecoder &Decoder, const char *Name,
                      TIOpcode ExpectedOpcode,
                      const std::vector<Operand> &ExpectedOperands) {
  uint8_t Bytes[TI32::WORD_SIZE] = {};
  File.read(reinterpret_cast<char *>(Bytes), sizeof(Bytes));
  require(static_cast<bool>(File),
          std::string(Name) + ": instruction is missing from binary");

  TIInstruction Instruction = Decoder.decode(decodeLittleEndianWord(Bytes));
  require(Instruction.OpCode == ExpectedOpcode,
          std::string(Name) + ": incorrect opcode");
  require(Instruction.Operands.size() == ExpectedOperands.size(),
          std::string(Name) + ": incorrect operand count");
  require(Instruction.Execute != nullptr,
          std::string(Name) + ": execute handler is missing");

  std::size_t Index = 0;
  for (const Operand &Expected : ExpectedOperands) {
    const Operand &Actual = Instruction.Operands[Index];
    require(Actual.Type == Expected.Type,
            std::string(Name) + ": incorrect operand type");
    require(Actual.Value == Expected.Value,
            std::string(Name) + ": incorrect operand value");
    ++Index;
  }
}

void testDecoder(const std::string &Filename) {
  std::ifstream File(Filename, std::ios::binary);
  require(File.is_open(), "Cannot open decoder test binary");

  TIDecoder Decoder{};

  checkInstruction(File, Decoder, "ST", TIOpcode::ST,
                   {
                       {OperandType::REGISTER,  2                        },
                       {OperandType::REGISTER,  1                        },
                       {OperandType::IMMEDIATE, static_cast<uint32_t>(-4)}
  });
  checkInstruction(File, Decoder, "ADDI", TIOpcode::ADDI,
                   {
                       {OperandType::REGISTER,  2                        },
                       {OperandType::REGISTER,  1                        },
                       {OperandType::IMMEDIATE, static_cast<uint32_t>(-7)}
  });
  checkInstruction(File, Decoder, "OR", TIOpcode::OR,
                   {
                       {OperandType::REGISTER, 3},
                       {OperandType::REGISTER, 1},
                       {OperandType::REGISTER, 2}
  });
  checkInstruction(File, Decoder, "LD register", TIOpcode::LDreg,
                   {
                       {OperandType::REGISTER, 4},
                       {OperandType::REGISTER, 1},
                       {OperandType::REGISTER, 3}
  });
  checkInstruction(File, Decoder, "J", TIOpcode::J,
                   {
                       {OperandType::IMMEDIATE, 15}
  });
  checkInstruction(File, Decoder, "BEQ", TIOpcode::BEQ,
                   {
                       {OperandType::REGISTER,  1 },
                       {OperandType::REGISTER,  2 },
                       {OperandType::IMMEDIATE, 10}
  });
  checkInstruction(File, Decoder, "CLZ", TIOpcode::CLZ,
                   {
                       {OperandType::REGISTER, 3},
                       {OperandType::REGISTER, 1}
  });
  checkInstruction(File, Decoder, "SSAT", TIOpcode::SSAT,
                   {
                       {OperandType::REGISTER,  2},
                       {OperandType::REGISTER,  1},
                       {OperandType::IMMEDIATE, 8}
  });
  checkInstruction(File, Decoder, "LD immediate", TIOpcode::LDimm,
                   {
                       {OperandType::REGISTER,  3                        },
                       {OperandType::REGISTER,  1                        },
                       {OperandType::IMMEDIATE, static_cast<uint32_t>(-4)}
  });
  checkInstruction(File, Decoder, "ADD", TIOpcode::ADD,
                   {
                       {OperandType::REGISTER, 3},
                       {OperandType::REGISTER, 1},
                       {OperandType::REGISTER, 2}
  });
  checkInstruction(File, Decoder, "SYSCALL", TIOpcode::SYSCALL,
                   {
                       {OperandType::IMMEDIATE, 0}
  });
  checkInstruction(File, Decoder, "BEXT", TIOpcode::BEXT,
                   {
                       {OperandType::REGISTER, 3},
                       {OperandType::REGISTER, 1},
                       {OperandType::REGISTER, 2}
  });
  checkInstruction(File, Decoder, "LI", TIOpcode::LI,
                   {
                       {OperandType::REGISTER,  2                        },
                       {OperandType::IMMEDIATE, static_cast<uint32_t>(-1)}
  });
  checkInstruction(File, Decoder, "RORI", TIOpcode::RORI,
                   {
                       {OperandType::REGISTER,  2},
                       {OperandType::REGISTER,  1},
                       {OperandType::IMMEDIATE, 7}
  });
  checkInstruction(File, Decoder, "STP", TIOpcode::STP,
                   {
                       {OperandType::REGISTER,  2                        },
                       {OperandType::REGISTER,  3                        },
                       {OperandType::REGISTER,  1                        },
                       {OperandType::IMMEDIATE, static_cast<uint32_t>(-4)}
  });

  TIInstruction Invalid = Decoder.decode(0x00000000);
  require(Invalid.OpCode == TIOpcode::INVALID,
          "Decoder accepted an invalid instruction");
}

} // namespace

int main(int ArgumentCount, char *Arguments[]) {
  if (ArgumentCount != 2) {
    std::cerr << "Usage: " << Arguments[0] << " <decoder.bin>\n";
    return 1;
  }

  try {
    testDecoder(Arguments[1]);
  } catch (const std::runtime_error &Error) {
    std::cerr << Error.what() << '\n';
    return 1;
  }

  std::cout << "All decoder tests passed\n";
  return 0;
}
