#pragma once

#include <array>
#include <cstdint>

struct TICpuState {
  std::array<uint32_t, 32> Registers{};
  uint32_t PC = 0;
};
