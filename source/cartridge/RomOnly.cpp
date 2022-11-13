#include "../../include/cartridge/RomOnly.h"
#include "../../include/state/EmulatorState.h"

namespace GameboyEmu::Cartridge {

	RomOnly::RomOnly(State::EmulatorState* state, byte* theRom, unsigned numbytes) :
		m_state(state), m_rom(theRom), m_bytes(numbytes),
		MemoryCard(span(theRom + 0x100, 0x014F - 0x0100 + 1)) {}

	byte RomOnly::Read(word address) {
		return m_rom[address];
	}

	void RomOnly::Write(word address, byte value) {
		LOG_WARN(m_state->GetLogger(), " No effect : writing ROM only cartridge\n");
	}

	byte RomOnly::GetCurrentBank(word at) const {
		return at <= 0x3FFF ? 0 : 1;
	}

	std::string RomOnly::DumpInfo() const {
		std::string ret = "Rom Only Cartridge\n" + MemoryCard::Dump();

		return ret;
	}

	bool RomOnly::SupportsSaves() const {
		return false;
	}

	const byte* RomOnly::GetRamBuffer() const {
		return nullptr;
	}

	void RomOnly::LoadRamSave(byte* buf) {
		(void)buf;
	}

	RomOnly::~RomOnly() {
		delete[] m_rom;
	}

	std::size_t RomOnly::DumpState(byte* buffer, std::size_t offset) {
		buffer[offset] = MemoryCard::GetType();

		return offset + 1;
	}

	std::size_t RomOnly::LoadState(byte* buffer, std::size_t offset) {
		byte type = buffer[offset];

		if (type != MemoryCard::GetType()) {
			throw std::runtime_error("Invalid cartridge type");
		}

		return offset + 1;
	}

	std::vector<RomOnly::replace_type> RomOnly::ApplyPatch(byte replace, word address, short compare) {
		std::vector<replace_type> ret;

		if (compare == -1 || m_rom[address] == compare) {
			byte old = m_rom[address];

			m_rom[address] = replace;

			if (address < 0x4000) {
				ret.push_back(replace_type(0, old));
			}
			else {
				ret.push_back(replace_type(1, old));
			}
		}

		return ret;
	}
	
	void RomOnly::RemovePatch(std::vector<RomOnly::replace_type> const& replaces, word address) {
		replace_type replace = replaces[0];

		m_rom[address] = replace.second;
	}

	byte RomOnly::ApplyShark(Cheats::GameShark const& shark) { return 0xFF; }
}