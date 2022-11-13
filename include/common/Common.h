#pragma once

#include <span>
#include <string_view>


//Utility types

/*
* Remember, bytes in memory use little-endian
*/

//Byte -> represents a byte in the memory (it is always unsigned)
using byte = unsigned char;

//Word -> two bytes
using word = unsigned short;

//Non-owning view of a range in memory
using span = std::span<unsigned char>;

/*
* Macros used for setting, resetting and getting
* Interrupt signal values from a register
* 
* The Flags register is as follows :
* 
* 1 byte
* Higher 4 bits are unused
* 
* - - - - J S T V
* 
* The priority is from right to left
*/
#define VBLANK_BIT_GET(reg) reg & 1
#define VBLANK_BIT_SET(reg) reg |= 0b1
#define VBLANK_BIT_RESET(reg) reg &= ~(0b1)

#define LCDSTAT_BIT_GET(reg) ((reg & 0b11) >> 1) & 1
#define LCDSTAT_BIT_SET(reg) reg |= 0b1 << 1
#define LCDSTAT_BIT_RESET(reg) reg &= ~(0b1 << 1)

#define TIMER_BIT_GET(reg) ((reg & 0b111) >> 2) & 1
#define TIMER_BIT_SET(reg) reg |= 0b1 << 2
#define TIMER_BIT_RESET(reg) reg &= ~(0b1 << 2)

#define SERIAL_BIT_GET(reg) ((reg & 0b1111) >> 3) & 1
#define SERIAL_BIT_SET(reg) reg |= 0b1 << 3
#define SERIAL_BIT_RESET(reg) reg &= ~(0b1 << 3)

#define JOYPAD_BIT_GET(reg) ((reg & 0b11111) >> 4) & 1
#define JOYPAD_BIT_SET(reg) reg |= 0b1 << 4
#define JOYPAD_BIT_RESET(reg) reg &= ~(0b1 << 4)

#define GET_BIT(val, pos) ((val >> pos) & 1)

inline void WriteWord(byte* buffer, std::size_t offset, word value) {
	buffer[offset++] = (value & 0xFF);
	buffer[offset] = ((value >> 8) & 0xFF);
}

inline word ReadWord(byte* buffer, std::size_t offset) {
	word ret = 0;

	ret = buffer[offset++];
	ret = (buffer[offset] << 8) | ret;

	return ret;
}

namespace StaticData {
	static constexpr uint32_t wram_size = 8 * 1024;
	static constexpr uint32_t hram_size = 0xFFFF - 0xFF80;
	static constexpr uint32_t vram_size = 8 * 1024;
	static constexpr uint32_t oam_size = 0xFE9F - 0xFE00 + 1;
	static constexpr uint32_t dmg_bootrom_size = 0xFF;
}