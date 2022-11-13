#pragma once

#include "../common/Common.h"

#define GET_HIGH(reg) ((reg >> 8) & 0xFF)
#define GET_LOW(reg) (reg & 0xFF)

#define SET_HIGH(reg, value) reg = (value << 8) | (reg & 0xFF)
#define SET_LOW(reg, value) reg = (reg & 0xFF00) | value

#define SET_FLAGS(z, s, h, c, reg) reg = (reg & 0xFF00) | ((z << 7) | (s << 6) | (h << 5) | (c << 4)) 

#define ZERO_FLAG(reg) (reg >> 7) & 1
#define SIGN_FLAG(reg) (reg >> 6) & 1
#define HALF_FLAG(reg) (reg >> 5) & 1
#define CARRY_FLAG(reg) (reg >> 4) & 1

namespace GameboyEmu::CPU {

	struct CpuContext {
		word ip; //instruction pointer
		word af;
		word bc;
		word de;
		word hl;
		word sp;

		bool enableInt;

		byte ei_delay;

		bool halted;
		bool haltBug;
	};

}