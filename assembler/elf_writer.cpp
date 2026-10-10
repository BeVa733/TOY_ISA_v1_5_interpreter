#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <elfio/elfio.hpp>

#include "../include/toy/toy_constants.hpp"

namespace {

constexpr uint32_t TEXT_ADDRESS      = 0;
constexpr uint32_t SECTION_ALIGNMENT = 4;

std::string readBinary(const std::string &Filename) {
  std::ifstream File(Filename, std::ios::binary | std::ios::ate);
  if (!File) {
    throw std::runtime_error("Cannot open section file: " + Filename);
  }

  std::streamoff Size = File.tellg();

  std::string Data(static_cast<std::size_t>(Size), '\0');
  File.seekg(0, std::ios::beg);

  File.read(Data.data(), Size);
  return Data;
}

void writeElf(const std::string &OutputFilename,
              const std::string &TextFilename,
              const std::string &DataFilename) {
  std::string Text = readBinary(TextFilename);
  std::string Data = readBinary(DataFilename);

  if (Text.empty() || Text.size() % SECTION_ALIGNMENT != 0) {
    throw std::runtime_error("Incorrect test section format");
  }

  uint32_t DataAddress = Text.size();

  ELFIO::elfio Writer{};
  Writer.create(ELFIO::ELFCLASS32, ELFIO::ELFDATA2LSB);
  Writer.set_os_abi(ELFIO::ELFOSABI_LINUX);
  Writer.set_type(ELFIO::ET_EXEC);
  Writer.set_machine(TI32::ELF_MACHINE);
  Writer.set_entry(TEXT_ADDRESS);

  ELFIO::section *TextSection = Writer.sections.add(".text");
  TextSection->set_type(ELFIO::SHT_PROGBITS);
  TextSection->set_flags(ELFIO::SHF_ALLOC | ELFIO::SHF_EXECINSTR);
  TextSection->set_address(TEXT_ADDRESS);
  TextSection->set_addr_align(SECTION_ALIGNMENT);
  TextSection->set_data(Text.data(), Text.size());

  ELFIO::segment *TextSegment = Writer.segments.add();
  TextSegment->set_type(ELFIO::PT_LOAD);
  TextSegment->set_virtual_address(TEXT_ADDRESS);
  TextSegment->set_physical_address(TEXT_ADDRESS);
  TextSegment->set_flags(ELFIO::PF_R | ELFIO::PF_X);
  TextSegment->set_align(SECTION_ALIGNMENT);
  TextSegment->add_section(TextSection, SECTION_ALIGNMENT);

  ELFIO::section *DataSection = Writer.sections.add(".data");
  DataSection->set_type(ELFIO::SHT_PROGBITS);
  DataSection->set_flags(ELFIO::SHF_ALLOC | ELFIO::SHF_WRITE);
  DataSection->set_address(DataAddress);
  DataSection->set_addr_align(SECTION_ALIGNMENT);
  DataSection->set_data(Data.data(), Data.size());

  ELFIO::segment *DataSegment = Writer.segments.add();
  DataSegment->set_type(ELFIO::PT_LOAD);
  DataSegment->set_virtual_address(DataAddress);
  DataSegment->set_physical_address(DataAddress);
  DataSegment->set_flags(ELFIO::PF_R | ELFIO::PF_W);
  DataSegment->set_align(SECTION_ALIGNMENT);
  DataSegment->add_section(DataSection, SECTION_ALIGNMENT);

  if (!Writer.save(OutputFilename)) {
    throw std::runtime_error("Cannot write ELF file: " + OutputFilename);
  }
}

} // namespace

int main(int ArgumentCount, char *Arguments[]) {
  if (ArgumentCount != 4) {
    std::cerr << "Usage: " << Arguments[0]
              << " <output.elf> <text.bin> <data.bin>\n";
    return 1;
  }

  try {
    writeElf(Arguments[1], Arguments[2], Arguments[3]);
  } catch (const std::exception &Error) {
    std::cerr << "[ELF WRITER] " << Error.what() << '\n';
    return 1;
  }

  return 0;
}
