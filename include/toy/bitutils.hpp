#pragma once

#include <cassert>
#include <cstdint>

#include "toy_constants.hpp"

inline constexpr uint32_t makeMask(uint32_t Width) {
  assert(Width <= TI32::WORD_BIT_COUNT && "Incorrect mask width");

  if (Width == TI32::WORD_BIT_COUNT) {
    return ~uint32_t{0};
  }

  return (uint32_t{1} << Width) - 1;
}

inline constexpr uint32_t extractBits(uint32_t Word, uint32_t Offset,
                                      uint32_t Width) {
  assert(Offset <= TI32::WORD_BIT_COUNT && "Incorrect bit offset");
  assert(Width <= TI32::WORD_BIT_COUNT - Offset && "Incorrect field width");

  if (Offset == TI32::WORD_BIT_COUNT) {
    return 0;
  }

  return (Word >> Offset) & makeMask(Width);
}

inline constexpr uint32_t signExtend(uint32_t Value, uint32_t Width) {
  assert(Width != 0 && Width <= TI32::WORD_BIT_COUNT && "Incorrect width");

  uint32_t Mask = makeMask(Width);
  Value &= Mask;

  if (Width == TI32::WORD_BIT_COUNT) {
    return Value;
  }

  uint32_t SignBit = uint32_t{1} << (Width - 1);
  if ((Value & SignBit) != 0) {
    Value |= ~Mask;
  }

  return Value;
}
