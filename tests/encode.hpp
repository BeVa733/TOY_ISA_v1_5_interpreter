#pragma once

#include <cstdint>

inline uint32_t encodeSt(uint32_t Rt, uint32_t Base, int32_t Immediate) {
  return (0x20u << 26) | (Base << 21) | (Rt << 16) |
         (static_cast<uint32_t>(Immediate) & 0x3FFFu);
}

inline uint32_t encodeAddi(uint32_t Rt, uint32_t Rs, int32_t Immediate) {
  return (0x17u << 26) | (Rs << 21) | (Rt << 16) |
         (static_cast<uint32_t>(Immediate) & 0xFFFFu);
}

inline uint32_t encodeOr(uint32_t Rd, uint32_t Rs, uint32_t Rt) {
  return (Rs << 21) | (Rt << 16) | (Rd << 11) | 0x1Au;
}

inline uint32_t encodeLdReg(uint32_t Rt, uint32_t Base, uint32_t Rm) {
  return (0x03u << 26) | (Base << 21) | (Rt << 16) | (0x03u << 14) | Rm;
}

inline uint32_t encodeJ(uint32_t InstructionIndex) {
  return (0x3Fu << 26) | (InstructionIndex & 0x03FFFFFFu);
}

inline uint32_t encodeBeq(uint32_t Rs, uint32_t Rt, int32_t Offset) {
  return (0x10u << 26) | (Rs << 21) | (Rt << 16) |
         (static_cast<uint32_t>(Offset) & 0xFFFFu);
}

inline uint32_t encodeClz(uint32_t Rd, uint32_t Rs) {
  return (Rd << 21) | (Rs << 16) | 0x32u;
}

inline uint32_t encodeSsat(uint32_t Rd, uint32_t Rs, uint32_t Immediate) {
  return (0x08u << 26) | (Rd << 21) | (Rs << 16) | ((Immediate & 0x1Fu) << 11);
}

inline uint32_t encodeLdImm(uint32_t Rt, uint32_t Base, int32_t Immediate) {
  return (0x33u << 26) | (Base << 21) | (Rt << 16) |
         (static_cast<uint32_t>(Immediate) & 0x3FFFu);
}

inline uint32_t encodeAdd(uint32_t Rd, uint32_t Rs, uint32_t Rt) {
  return (Rs << 21) | (Rt << 16) | (Rd << 11) | 0x0Cu;
}

inline uint32_t encodeSyscall(uint32_t Code = 0) {
  return ((Code & 0x000FFFFFu) << 6) | 0x1Eu;
}

inline uint32_t encodeBext(uint32_t Rd, uint32_t Rs1, uint32_t Rs2) {
  return (Rd << 21) | (Rs1 << 16) | (Rs2 << 11) | 0x34u;
}

inline uint32_t encodeLi(uint32_t Rt, int32_t Immediate) {
  return (0x0Bu << 26) | (Rt << 16) |
         (static_cast<uint32_t>(Immediate) & 0xFFFFu);
}

inline uint32_t encodeRori(uint32_t Rd, uint32_t Rs, uint32_t Immediate) {
  return (0x1Du << 26) | (Rd << 21) | (Rs << 16) | ((Immediate & 0x1Fu) << 11);
}

inline uint32_t encodeStp(uint32_t Rt1, uint32_t Rt2, uint32_t Base,
                          int32_t Offset) {
  return (0x15u << 26) | (Base << 21) | (Rt1 << 16) | (Rt2 << 11) |
         (static_cast<uint32_t>(Offset) & 0x07FFu);
}
