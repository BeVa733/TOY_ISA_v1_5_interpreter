#include "../include/toy/execution_context.hpp"
#include "encode.hpp"

#include <bitset>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool Condition, const std::string &Message) {
  if (!Condition) {
    throw std::runtime_error(Message);
  }
}

void writeProgram(const std::filesystem::path &Path,
                  const std::vector<uint32_t> &Words) {
  std::ofstream File(Path, std::ios::binary | std::ios::trunc);

  require(File.is_open(), "Cannot create instruction test binary");

  for (uint32_t Word : Words) {
    char Bytes[4]{static_cast<char>(Word), static_cast<char>(Word >> 8),
                  static_cast<char>(Word >> 16), static_cast<char>(Word >> 24)};
    File.write(Bytes, sizeof(Bytes));
  }

  File.close();
  require(static_cast<bool>(File), "Cannot write instruction test binary");
}

void loadProgram(ExecutionContext &Context, const std::filesystem::path &Path,
                 const std::vector<uint32_t> &Words) {
  writeProgram(Path, Words);
  Context.loadBinary(Path.string());
}

std::string formatInstruction(uint32_t Address, uint32_t Word) {
  std::ostringstream Message;
  Message << "address=0x" << std::hex << std::setfill('0') << std::setw(8)
          << Address << ", word=0x" << std::setw(8) << Word
          << ", bits=" << std::bitset<32>(Word);
  return Message.str();
}

void runBlock(ExecutionContext &Context) {
  uint32_t StartAddress = Context.cpu().PC;

  try {
    Context.step();
  } catch (const std::exception &Error) {
    uint32_t FailedAddress = Context.cpu().PC;
    uint32_t Word          = Context.memory().read32(FailedAddress);
    throw std::runtime_error(
        "block at address 0x" + std::to_string(StartAddress) + ", " +
        formatInstruction(FailedAddress, Word) + "\n" + Error.what());
  }
}

void dumpProgram(const std::filesystem::path &Path, std::ostream &Output) {
  std::ifstream File(Path, std::ios::binary);
  if (!File) {
    Output << "Cannot open test binary for diagnostic output: " << Path << '\n';
    return;
  }

  Output << "Binary: " << Path << '\n';
  uint32_t Address = 0;
  while (true) {
    unsigned char Bytes[4]{};
    File.read(reinterpret_cast<char *>(Bytes), sizeof(Bytes));
    if (File.gcount() == 0) {
      break;
    }
    if (File.gcount() != sizeof(Bytes)) {
      Output << "Incomplete word at address 0x" << std::hex << Address << '\n';
      break;
    }

    uint32_t Word = static_cast<uint32_t>(Bytes[0]) |
                    (static_cast<uint32_t>(Bytes[1]) << 8) |
                    (static_cast<uint32_t>(Bytes[2]) << 16) |
                    (static_cast<uint32_t>(Bytes[3]) << 24);
    Output << "  " << formatInstruction(Address, Word) << '\n';
    Address += 4;
  }
}

using TestFunction = void (*)(const std::filesystem::path &);

void runTest(const std::string &Name, TestFunction Test,
             const std::filesystem::path &Path) {
  try {
    Test(Path);
  } catch (const std::exception &Error) {
    throw std::runtime_error("[TEST] " + Name + " failed\n" + Error.what());
  }
}

void testLi(const std::filesystem::path &Path) {
  ExecutionContext Context(64);
  loadProgram(Context, Path, {encodeLi(31, -1)});
  runBlock(Context);
  require(Context.cpu().Registers[31] == 0xFFFFFFFFu, "LI failed");
  require(Context.cpu().PC == 4, "LI did not advance PC");
}

void testAdd(const std::filesystem::path &Path) {
  ExecutionContext Context(64);
  loadProgram(Context, Path,
              {encodeLi(1, -1), encodeLi(2, 1), encodeAdd(3, 1, 2)});
  runBlock(Context);
  require(Context.cpu().Registers[3] == 0, "ADD wraparound failed");
}

void testAddi(const std::filesystem::path &Path) {
  ExecutionContext Context(64);
  loadProgram(Context, Path, {encodeLi(1, 5), encodeAddi(2, 1, -7)});
  runBlock(Context);
  require(Context.cpu().Registers[2] == 0xFFFFFFFEu,
          "ADDI negative immediate failed");
}

void testOr(const std::filesystem::path &Path) {
  ExecutionContext Context(64);
  loadProgram(Context, Path,
              {encodeLi(1, 0x0F00), encodeLi(2, 0x00F0), encodeOr(3, 1, 2)});
  runBlock(Context);
  require(Context.cpu().Registers[3] == 0x0FF0, "OR failed");
}

void testSt(const std::filesystem::path &Path) {
  ExecutionContext Context(128);
  loadProgram(Context, Path,
              {encodeLi(1, 80), encodeLi(2, 0x1234), encodeSt(2, 1, -4)});
  runBlock(Context);
  require(Context.memory().read32(76) == 0x1234, "ST failed");
}

void testLdImm(const std::filesystem::path &Path) {
  ExecutionContext Context(128);
  loadProgram(Context, Path,
              {encodeLi(1, 80), encodeLi(2, 0x2345), encodeSt(2, 1, -4),
               encodeLdImm(3, 1, -4)});
  runBlock(Context);
  require(Context.cpu().Registers[3] == 0x2345, "LD immediate failed");
}

void testLdReg(const std::filesystem::path &Path) {
  ExecutionContext Context(128);
  loadProgram(Context, Path,
              {encodeLi(1, 80), encodeLi(2, 0x3456), encodeSt(2, 1, 0),
               encodeLi(3, 0), encodeLdReg(4, 1, 3)});
  runBlock(Context);
  require(Context.cpu().Registers[4] == 0x3456, "LD register failed");
}

void testStp(const std::filesystem::path &Path) {
  ExecutionContext Context(128);
  loadProgram(Context, Path,
              {encodeLi(1, 80), encodeLi(2, 0x1111), encodeLi(3, 0x2222),
               encodeStp(2, 3, 1, 4)});
  runBlock(Context);
  require(Context.memory().read32(84) == 0x1111, "STP first word failed");
  require(Context.memory().read32(88) == 0x2222, "STP second word failed");
}

void testBeq(const std::filesystem::path &Path) {
  ExecutionContext Taken(64);
  loadProgram(Taken, Path,
              {encodeLi(1, 7), encodeLi(2, 7), encodeBeq(1, 2, 2)});
  runBlock(Taken);
  require(Taken.cpu().PC == 16, "BEQ taken failed");

  ExecutionContext NotTaken(64);
  loadProgram(NotTaken, Path,
              {encodeLi(1, 7), encodeLi(2, 8), encodeBeq(1, 2, 2)});
  runBlock(NotTaken);
  require(NotTaken.cpu().PC == 12, "BEQ not taken failed");
}

void testJ(const std::filesystem::path &Path) {
  ExecutionContext Context(64);
  loadProgram(Context, Path, {encodeJ(5)});
  runBlock(Context);
  require(Context.cpu().PC == 20, "J failed");
}

void testClz(const std::filesystem::path &Path) {
  ExecutionContext Context(64);
  loadProgram(Context, Path,
              {encodeClz(1, 0), encodeLi(2, 1), encodeClz(3, 2)});
  runBlock(Context);
  require(Context.cpu().Registers[1] == 32, "CLZ zero failed");
  require(Context.cpu().Registers[3] == 31, "CLZ one failed");
}

void testSsat(const std::filesystem::path &Path) {
  ExecutionContext Context(64);
  loadProgram(Context, Path,
              {encodeLi(1, 200), encodeSsat(2, 1, 8), encodeLi(3, -200),
               encodeSsat(4, 3, 8)});
  runBlock(Context);
  require(Context.cpu().Registers[2] == 127, "SSAT upper bound failed");
  require(Context.cpu().Registers[4] == 0xFFFFFF80u, "SSAT lower bound failed");
}

void testRori(const std::filesystem::path &Path) {
  ExecutionContext Context(64);
  loadProgram(Context, Path,
              {encodeLi(1, 1), encodeRori(2, 1, 1), encodeRori(3, 1, 0),
               encodeRori(4, 1, 31)});
  runBlock(Context);
  require(Context.cpu().Registers[2] == 0x80000000u, "RORI one failed");
  require(Context.cpu().Registers[3] == 1, "RORI zero failed");
  require(Context.cpu().Registers[4] == 2, "RORI 31 failed");
}

void testBext(const std::filesystem::path &Path) {
  ExecutionContext Context(64);
  loadProgram(Context, Path,
              {encodeLi(1, 0x00D6), encodeLi(2, 0x00AC), encodeBext(3, 1, 2)});
  runBlock(Context);
  require(Context.cpu().Registers[3] == 9, "BEXT failed");
}

void testSyscall(const std::filesystem::path &Path) {
  ExecutionContext Context(64);
  loadProgram(Context, Path,
              {encodeLi(8, 60), encodeLi(0, 42), encodeSyscall()});
  runBlock(Context);
  require(Context.state().Status == ExecutionStatus::HALTED,
          "SYSCALL did not halt");
  require(Context.state().ExitCode == 42, "SYSCALL returned wrong exit code");
  require(Context.cpu().PC == 8, "SYSCALL changed PC after exit");
}

} // namespace

int main() {
  std::filesystem::path Path = std::filesystem::temp_directory_path() /
                               ("toy_isa_instruction_tests.bin");

  try {
    runTest("LI", testLi, Path);
    runTest("ADD", testAdd, Path);
    runTest("ADDI", testAddi, Path);
    runTest("OR", testOr, Path);
    runTest("ST", testSt, Path);
    runTest("LD immediate", testLdImm, Path);
    runTest("LD register", testLdReg, Path);
    runTest("STP", testStp, Path);
    runTest("BEQ", testBeq, Path);
    runTest("J", testJ, Path);
    runTest("CLZ", testClz, Path);
    runTest("SSAT", testSsat, Path);
    runTest("RORI", testRori, Path);
    runTest("BEXT", testBext, Path);
    runTest("SYSCALL", testSyscall, Path);
    std::filesystem::remove(Path);
  } catch (const std::exception &Error) {
    std::cerr << Error.what() << '\n';
    dumpProgram(Path, std::cerr);
    std::cerr << "Failed binary was preserved for inspection\n";
    return 1;
  }

  std::cout << "All 15 ISA instruction tests passed\n";
  return 0;
}
