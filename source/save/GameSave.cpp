#include "../../include/save/GameSave.h"
#include "../../include/cartridge/MemoryCard.h"

#include <chrono>

#include <filesystem>
#include <fstream>

namespace GameboyEmu::Saves {
	void dumpHeader(Cartridge::MemoryCard* card, std::ofstream& file) {
		save_header header{};

		std::fill_n(header.title, 16, '\0');

		header.magic = 0b10101010;

		auto now = std::chrono::system_clock::now();

		uint64_t nano = now.time_since_epoch().count();

		header.modified = nano;

		auto title = card->GetTitle();

		std::copy_n(title.data(), title.length(), header.title);

		header.size_kb = Cartridge::getRamKb(card->GetRamSize());

		file.write(reinterpret_cast<char*>(&header), sizeof(header));
	}

	std::pair<bool, std::string> checkHeader(Cartridge::MemoryCard* card, std::ifstream& file) {
		save_header header{};

		file.read(reinterpret_cast<char*>(&header), sizeof(header));

		if (!file.good()) {
			return std::pair(false, "Header read failed");
		}

		std::string title(header.title, 16);

		if (header.magic != 0b10101010) {
			return std::pair(false, "Invalid signature");
		}

		auto view = card->GetTitle();

		std::string real_title = std::string(view.data(), 16);

		if (title != real_title) {
			return std::pair(false, "Invalid title name");
		}

		if (header.size_kb != Cartridge::getRamKb(card->GetRamSize())) {
			return std::pair(false, "Invalid size");
		}

		return std::pair(true, "Header ok");
	}

	std::pair<bool, std::string> SaveGame(Cartridge::MemoryCard* card, std::string const& path) {
		if (!card->SupportsSaves()) {
			return std::pair(false, "Cartridge does not support saves");
		}

		auto size_kb = Cartridge::getRamKb(card->GetRamSize());
		
		std::ofstream save(path, std::ios::out);

		if (!save.is_open()) {
			return std::pair(false, "Cannot create file");
		}

		save.close();

		save.open(path, std::ios::out | std::ios::binary);

		dumpHeader(card, save);

		const byte* buf = card->GetRamBuffer();

		save.write(reinterpret_cast<const char*>(buf), (uint64_t)size_kb * 1024);

		return std::pair(true, "Game saved");
	}

	std::pair<bool, std::string> LoadGame(Cartridge::MemoryCard* card, std::string const& path) {
		if (!card->SupportsSaves()) {
			return std::pair(false, "Cartridge does not support saves");
		}
		
		if (!std::filesystem::exists(path)) {
			return std::pair(false, "File does not exist");
		}

		if (!std::filesystem::is_regular_file(path)) {
			return std::pair(false, "File is not a regular file");
		}

		std::ifstream save(path, std::ios::in | std::ios::binary);

		if (!save.is_open()) {
			return std::pair(false, "Could not open file");
		}

		auto check = checkHeader(card, save);

		if (!check.first) {
			return std::pair(false, check.second);
		}

		uint64_t numkb = Cartridge::getRamKb(card->GetRamSize());

		byte* data = new byte[numkb * 1024];

		save.read(reinterpret_cast<char*>(data), numkb * 1024);

		if (!save.good()) {
			return std::pair(false, "Could not read enough bytes");
		}

		card->LoadRamSave(data);

		return std::pair(true, "Save loaded");
	}
}