#pragma once

#include "../include/toy/execution_state.hpp"

#include <stdexcept>
#include <string>

inline void require(bool Condition, const std::string &Message) {
  if (!Condition) {
    throw std::runtime_error(Message);
  }
}

template <typename TAction>
void requireError(ErrorCode Expected, TAction Action, const char *Description) {
  try {
    Action();
  } catch (const SimulationException &Error) {
    require(Error.code() == Expected,
            std::string(Description) + ": incorrect error code");
    return;
  }

  throw std::runtime_error(std::string(Description) +
                           ": expected error was not thrown");
}
