#pragma once

#include "../common/Common.h"
#include <functional>

namespace GameboyEmu::State {
	class EmulatorState;
}

namespace GameboyEmu::Debugger {
	using Callbak = std::function<void(State::EmulatorState*)>;

	enum class WatchType {
		read = 0x0,
		write = 0x1,
		readwrite = 0x2
	};

	struct Watchpoint {
		word address;
		bool enabled;
		WatchType type; 
		Callbak callback;
	};
}