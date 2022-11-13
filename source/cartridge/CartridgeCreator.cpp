#include "../../include/cartridge/CartridgeCreator.h"

#include <utility>
#include <fstream>

#include "../../include/cartridge/RomOnly.h"
#include "../../include/cartridge/Mbc1.h"
#include "../../include/cartridge/Mbc3.h"
#include "../../include/cartridge/CardUtils.h"

namespace GameboyEmu::Cartridge {

	std::pair< MemoryCard*, std::string > CreateCartridge(std::filesystem::path const& path, State::EmulatorState* ctx) {
		if (!std::filesystem::exists(path)) {
			return std::pair(nullptr, "File does not exist");
		}

		if (!std::filesystem::is_regular_file(path)) {
			return std::pair(nullptr, "File is not a regular file");
		}

		//open file in read binary mode
		std::ifstream file(path, std::ios::in | std::ios::binary);

		//file is not readable 
		if (!file.good())
			return std::pair(nullptr, "Cannot read file");

		//set position at the end of the file and read the index value
		file.seekg(0, std::ios::end);

		std::streampos sz = file.tellg();

		//reset the position
		file.seekg(0, std::ios::beg);
		////////////////////////

		//allocate buffer
		byte* data = new byte[sz];

		//copy all the file into memory
		file.read((char*)data, sz);

		if (sz < 0x014F)
			return std::pair(nullptr, "File is too small");

		MemoryCard* ret = nullptr;
		std::string message = "";

		if (data[0x0143] == 0xC0) {
			return std::pair(nullptr, "Cartridge requires CGB functions");
		}

		switch (data[0x0147])
		{
		case 0x00:
			ret = new RomOnly(ctx, data, sz);
			break;

		case 0x01:
		case 0x02:
		case 0x03:
			ret = new Mbc1(ctx, data, sz);
			break;

		case 0x0F:
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
			ret = new Mbc3(ctx, data, sz);
			break;

		default:
			message = "Invalid or unimplemented cartridge type";
			break;
		}

		return std::pair(ret, message);
	}
}