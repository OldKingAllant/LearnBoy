#include "../../include/cartridge/Mbc3.h"

namespace GameboyEmu::Cartridge {
	Mbc3::Mbc3(State::EmulatorState* state, byte* data, unsigned numb) :
		m_state(state), m_rom(data), m_bank_number(1), 
		m_sram(nullptr), m_ram_bank_number(), m_enable_rtc_ram(false), 
		m_total_banks(), m_total_ram_banks(), m_rtc_reg_select(),
		m_rtc_or_ram(false), m_rtc(),
		MemoryCard(span(data + 0x100, 0x014F - 0x100 + 1))
	{
		word ramkb = getRamKb(MemoryCard::GetRamSize());

		m_total_banks = numb / (16 * 1024);
		m_total_ram_banks = ramkb / 8;

		m_sram = new byte[ramkb * 1024];
	}

	byte Mbc3::Read(word address) {
		if (address < 0x4000) {
			return m_rom[address];
		}
		else if (address < 0x8000) {
			return m_rom[(address - 0x4000) + (m_bank_number * 0x4000)];
		} 
		else if (address >= 0xA000 && address < 0xC000) {
			if (!m_enable_rtc_ram)
				return 0xFF;

			if (m_rtc_or_ram) {
				return m_rtc.ReadRegister(m_rtc_reg_select);
			}

			return m_sram[(address - 0xA000) + (m_ram_bank_number * 0x2000)];
		}

		return 0xFF;
	}

	void Mbc3::Write(word address, byte value) {
		if (address < 0x2000) {
			m_enable_rtc_ram = (value & 0x0F) == 0x0A;
		}
		else if (address < 0x4000) {
			m_bank_number = value;

			if (m_bank_number == 0)
				m_bank_number = 1;

			m_bank_number &= ~m_total_banks;
		}
		else if (address < 0x6000) {
			if (value >= 0x8 && value <= 0xC) {
				m_rtc_or_ram = true;
				m_rtc_reg_select = value - 0x08;
			}
			else {
				m_rtc_or_ram = false;
				m_ram_bank_number = value;
				m_ram_bank_number &= ~m_total_ram_banks;
			}
		}
		else if (address < 0x8000) {
			m_rtc.LatchClock(value);
		}
		else if (address >= 0xA000 && address < 0xC000) {
			if (!m_enable_rtc_ram)
				return;

			if (m_rtc_or_ram) {
				m_rtc.WriteRegister(m_rtc_reg_select, value);
				return;
			}
				
			m_sram[(address - 0xA000) + (m_ram_bank_number * 0x2000)] =
				value;
		}
	}

	byte Mbc3::GetCurrentBank(word at) const {
		if (at < 0x4000)
			return 0;

		return m_bank_number;
	}

	bool Mbc3::SupportsSaves() const {
		return m_total_ram_banks != 0;
	}

	const byte* Mbc3::GetRamBuffer() const {
		return m_sram;
	}

	void Mbc3::LoadRamSave(byte* buf) {
		std::copy_n(buf, m_total_ram_banks * 0x2000, m_sram);
	}

	Mbc3::~Mbc3() {
		delete[] m_sram;
		delete[] m_rom;
	}

	std::size_t Mbc3::DumpState(byte* buffer, std::size_t offset) {
		_ASSERT(0 && "Unimplemented");

		return offset;
	}

	std::size_t Mbc3::LoadState(byte* buffer, std::size_t offset) {
		_ASSERT(0 && "Unimplemented");

		return offset;
	}

	std::vector<Mbc3::replace_type> Mbc3::ApplyPatch(byte replace, word address, short compare) {
		std::vector<replace_type> ret;

		if (address < 0x4000) {
			//Only rom bank 00 can be
			//mapped to 0x0000 - 0x3FFF
			if (compare == -1 || m_rom[address] == compare) {
				byte old_value = m_rom[address];

				m_rom[address] = replace;

				ret.push_back(replace_type(0, old_value));
			}
		}
		else {
			for (byte bank = 1; bank < m_total_banks; bank++) {
				uint64_t pos = ((uint64_t)bank * 0x4000) + (
					(uint64_t)address - 0x4000);

				if (compare == -1 || m_rom[address] == compare) {
					byte old_value = m_rom[address];

					m_rom[address] = replace;

					ret.push_back(replace_type(bank, old_value));
				}
			}
		}

		return ret;
	}

	void Mbc3::RemovePatch(std::vector<Mbc3::replace_type> const& replaces, word address) {
		for (auto const& replace : replaces) {
			byte old_value = replace.second;
			word banknum = replace.first;

			address = address > 0x3FFF ? address - 0x4000 : address;

			uint64_t pos = ((uint64_t)banknum * 0x4000) + address;

			m_rom[pos] = old_value;
		}
	}

	byte Mbc3::ApplyShark(Cheats::GameShark const& shark) {
		if (shark.bank_number > m_total_banks) {
			return 0xFF;
		}

		uint64_t address = ((uint64_t)shark.address - 0xA000) +
			((uint64_t)shark.bank_number * 0x2000);

		byte old = m_sram[address];

		m_sram[address] = shark.new_data;

		return old;
	}
}