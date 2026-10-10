#pragma once

#include <cstdint>

namespace TI32 {

inline constexpr uint16_t ELF_MACHINE = 0x6767;

inline constexpr uint32_t WORD_SIZE        = 4;
inline constexpr uint32_t WORD_BIT_COUNT   = WORD_SIZE * 8;
inline constexpr uint32_t INSTRUCTION_SIZE = 4;
inline constexpr uint32_t REGISTER_COUNT   = 32;

} // namespace TI32
