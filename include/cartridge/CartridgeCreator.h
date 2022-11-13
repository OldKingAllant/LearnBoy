#pragma once

#include <filesystem>
#include "./MemoryCard.h"

namespace GameboyEmu::State {
	class EmulatorState;
}

namespace GameboyEmu::Cartridge {

	/*
	* If file at path exists, is a regular file and is readable,
	* creates a new MemoryCard which types depends on the
	* type found in the file.
	*/
	std::pair< MemoryCard*, std::string > CreateCartridge(std::filesystem::path const& path, State::EmulatorState* ctx);

}