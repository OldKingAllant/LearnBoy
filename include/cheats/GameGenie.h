#pragma once

#include "../common/Common.h"
#include <optional>

namespace GameboyEmu::Cheats {
	struct GameGenie {
		std::string cheat_string;
		byte replace_value;
		word address;
		std::optional<byte> compare_value;
	};
}