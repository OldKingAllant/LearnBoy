#include "../../include/cpu/Disasm.h"
#include "../../include/memory/Memory.h"

#include <format>

namespace GameboyEmu::CPU {

	std::pair< std::string, byte > Disassemble(word address, Mem::Memory* mem) {
		word oldaddress = address;

		byte inst = mem->Read(address);

		std::string ret = "";

		const byte* paramTable;

		if (inst == 0xCB) {
			ret += "CB ";

			address++;

			inst = mem->Read(address);

			ret += cbInstructions::disasm[inst];

			paramTable = cbInstructions::numParams;
		}
		else {
			ret += normalInstructions::disasm[inst];

			paramTable = normalInstructions::numParams;
		}

		byte numParams = paramTable[inst];

		while (numParams-- && (unsigned)address + 1 <= 0xFFFF) {
			address++;

			ret += std::format(" 0x{:x}", mem->Read(address));
		}

		return std::pair(ret, address - oldaddress + 1);
	}
}