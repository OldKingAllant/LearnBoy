#pragma once

#include "../common/Common.h"

namespace GameboyEmu::Cheats {
	struct GameShark {
		byte bank_number;
		byte new_data;
		word address;
		byte old_value;
	};
}