#pragma once

#include <array>
#include <cstdint>

#include "toy_constants.hpp"

struct TICpuState {
  std::array<uint32_t, TI32::REGISTER_COUNT> Registers{};
  uint32_t PC = 0;
};
