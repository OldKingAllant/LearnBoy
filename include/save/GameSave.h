#pragma once

#include "../common/Common.h"

namespace GameboyEmu::Cartridge {
	class MemoryCard;
}

namespace GameboyEmu::Saves {
	struct alignas(8) save_header {
		uint8_t magic;
		uint64_t modified;
		char title[16];
		uint8_t size_kb;
	};

	std::pair<bool, std::string> SaveGame(Cartridge::MemoryCard* card, std::string const& path);

	std::pair<bool, std::string> LoadGame(Cartridge::MemoryCard* card, std::string const& path);
}