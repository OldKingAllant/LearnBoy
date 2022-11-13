#pragma once

#include "../common/Common.h"

namespace GameboyEmu::Debugger {
	struct Breakpoint {
		byte bank;
		word address;
		bool enabled;
		word hitrate;
		word require_hitrate;
	};
}