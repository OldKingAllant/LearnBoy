#include "../../include/save/Savestate.h"
#include "../../include/state/EmulatorState.h"

#include "../../include/cartridge/MemoryCard.h"
#include "../../include/cpu/Cpu.h"
#include "../../include/memory/Memory.h"
#include "../../include/graphics/ppu/PPU.h"
#include "../../include/timing/Timer.h"
#include "../../include/datatransfer/Serial.h"
#include "../../include/sound/apu/APU.h"

#include <fstream>
#include <chrono>
#include <filesystem>

namespace GameboyEmu::Saves {
	std::pair<bool, std::string> SavestateSaveHeader(std::ofstream& file, State::EmulatorState* state) {
		byte magic = 0b11001100;

		auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();

		auto title_view = state->GetCard()->GetTitle();

		char title[16] = {};

		std::fill_n(title, 16, '\0');

		std::copy_n(title_view.data(), title_view.length(), title);

		char version[16] = {};

		std::fill_n(version, 16, '\0');

		version[0] = '1';
		version[1] = '.';
		version[2] = '0';

		file.write(reinterpret_cast<char*>(&magic), 1);
		file.write(reinterpret_cast<char*>(&now), sizeof(now));
		file.write(title, 16);
		file.write(version, 16);

		return std::pair(true, "");
	}

	std::pair<bool, std::string> SavestateLoadHeader(std::ifstream& file, State::EmulatorState* state) {
		byte magic = 0;
		uint64_t now = 0;
		char title_data[16] = {};
		char version[16] = {};

		auto title_view = state->GetCard()->GetTitle();

		file.read(reinterpret_cast<char*>(&magic), 1);
		file.read(reinterpret_cast<char*>(&now), sizeof(now));
		file.read(title_data, 16);
		file.read(version, 16);

		if (magic != 0b11001100) {
			return std::pair(false, "Invalid magic");
		}

		std::string title_saved = std::string(title_data, 16);
		std::string title_orig(16, '\0');

		std::copy_n(title_view.data(), title_view.length(), title_orig.begin());

		if (title_saved != title_orig) {
			return std::pair(false, "Invalid game title");
		}

		return std::pair(true, "");
	}

	std::pair<bool, std::string> SaveState(std::string const& to, State::EmulatorState* state) {
		std::ofstream save(to, std::ios::out);

		if (!save.is_open()) {
			return std::pair(false, "Could not create file");
		}

		save.close();

		save.open(to, std::ios::out | std::ios::binary);

		SavestateSaveHeader(save, state);

		auto cpu = state->GetCPU();
		auto mem = state->GetMemory();
		auto ppu = state->GetPPU();
		auto tim = state->GetTimer();
		auto ser = state->GetSerial();
		auto apu = state->GetAPU();
		auto car = state->GetCard();

		byte* megabuffer = new byte[256 * 1024];

		std::size_t offset = 0;

		offset = cpu->DumpState(megabuffer, offset);
		offset = mem->DumpState(megabuffer, offset);
		offset = ppu->DumpState(megabuffer, offset);
		offset = tim->DumpState(megabuffer, offset);
		offset = ser->DumpState(megabuffer, offset);
		offset = apu->DumpState(megabuffer, offset);
		offset = car->DumpState(megabuffer, offset);

		save.write(reinterpret_cast<char*>(megabuffer), 256 * 1024);

		delete[] megabuffer;

		return std::pair(true, "");
	}

	std::pair<bool, std::string> LoadState(std::string const& from, State::EmulatorState* state) {
		std::ifstream save;

		if (!std::filesystem::exists(from)) {
			return std::pair(false, "File does not exist");
		}

		if (!std::filesystem::is_regular_file(from)) {
			return std::pair(false, "File is not a regular file");
		}

		save.open(from, std::ios::in | std::ios::binary);

		if (!save.is_open()) {
			return std::pair(false, "Could not open file");
		}

		auto check = SavestateLoadHeader(save, state);

		if (!check.first) {
			return check;
		}

		auto cpu = state->GetCPU();
		auto mem = state->GetMemory();
		auto ppu = state->GetPPU();
		auto tim = state->GetTimer();
		auto ser = state->GetSerial();
		auto apu = state->GetAPU();
		auto car = state->GetCard();

		byte* megabuffer = new byte[256 * 1024];

		save.read(reinterpret_cast<char*>(megabuffer), 256 * 1024);

		std::size_t offset = 0;

		offset = cpu->LoadState(megabuffer, offset);
		offset = mem->LoadState(megabuffer, offset);
		offset = ppu->LoadState(megabuffer, offset);
		offset = tim->LoadState(megabuffer, offset);
		offset = ser->LoadState(megabuffer, offset);
		offset = apu->LoadState(megabuffer, offset);
		offset = car->LoadState(megabuffer, offset);

		delete[] megabuffer;

		return std::pair(true, "");
	}
}