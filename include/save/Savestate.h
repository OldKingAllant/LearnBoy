#pragma once

#include "../common/Common.h"

namespace GameboyEmu::State {
	class EmulatorState;
}

namespace GameboyEmu::Saves {
	std::pair<bool, std::string> SaveState(std::string const& to, State::EmulatorState* state);
	std::pair<bool, std::string> LoadState(std::string const& from, State::EmulatorState* state);
}