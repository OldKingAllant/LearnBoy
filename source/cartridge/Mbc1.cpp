#include "../../include/cartridge/Mbc1.h"
#include "../../include/state/EmulatorState.h"

namespace GameboyEmu::Cartridge {

	Mbc1::Mbc1(State::EmulatorState* state, byte* data, unsigned numb)
		: m_state(state), m_therom(data), m_numbytes(numb), m_second_bank_num(1),
		m_bank_num_hi(0), m_first_bank_num(0), m_enableRam(false), m_bankingMode(false), m_numbanks(0),
		m_rambank(0), m_numrambanks(0), m_sram(nullptr),
		MemoryCard(span(data + 0x100, 0x014F - 0x100 + 1)) {
		m_numbanks = m_numbytes / 0x4000;

		auto ramSz = MemoryCard::GetRamSize();

		word ramKb = getRamKb(ramSz);

		m_numrambanks = ramKb / 8;

		m_sram = new byte[ramKb * 1024];
	}

	/*
	* The MBC has two banking modes:
	* - Normal
	* - Advanced
	*
	* In normal mode, the range
	* 0x0000 - 0x3FFF Is locked to the first ROM
	* bank of the cartridge
	* 0xA000 - 0xBFFF Is also locked to RAM bank 0
	*
	* Selecting ROM banks 0x00, 0x20, 0x40, 0x60 is not possible
	*
	*
	*
	* In Advanced mode, the two ranges are mapped to
	* the bank selected with the two bits register
	* banknum_hi, and the range 0x4000 - 0x7FFF
	* is mapped normally
	*
	* Readable memory
	*
	* 0x0000 - 0x3FFF -> Rom bank X0, 0 in mode 0, X0 in mode 1
	* 0x4000 - 0x7FFF -> Selectable rom bank
	* 0xA000 - 0xBFFF -> RAM bank 00-03 if any
	*
	* Registers
	* 0x0000 - 0x1FFF -> Enable/disable ram
	*                    any value which low 4 bits are
	*                    0xA enables the RAM, any other value disables it
	* 0x2000 - 0x3FFF -> ROM bank number (5 bits, higher bits are discarded)
	* 0x4000 - 0x5FFF -> RAM bank number/upper 2 bits of rom bank
	* 0x6000 - 0x7FFF -> Switch banking mode
	*/


	byte Mbc1::Read(word address) {
		switch (address & 0xE000)
		{
		case 0x0000:
		case 0x2000:
		{
			//selected rom bank
			byte bank = 0;

			//if banking mode == 0x01, the 
			//current bank is selected using the
			//two bits of the secondary bank number register
			if (m_bankingMode) {
				bank = (m_bank_num_hi << 5) & (m_numbanks - 1);
			}
			//else, the value is locked to 0

			return m_therom[(bank * 0x4000) + address];
		}
		break;

		case 0x4000:
		case 0x6000:
		{
			//the selected bank is calculated using both registers
			byte bank = ((m_bank_num_hi << 5) + m_second_bank_num) & (m_numbanks - 1);
			return m_therom[(address - 0x4000) + (0x4000 * bank)];
		}
		break;

		case 0xA000: {
			if (!m_enableRam)
				return 0xFF;

			byte rambankSelect = 0;

			if (m_bankingMode) {
				rambankSelect = m_second_bank_num
					& (m_numrambanks - 1);
			}

			return m_sram[(address - 0xA000)
				+ (rambankSelect * 0x2000)];
		} break;

		default:
			LOG_WARN(m_state->GetLogger(), "Invalid read at {2:x}\n", address);
			break;
		}

		return 0x00;
	}

	/*
	* 0x0000 - 0x1FFF Enable/Disable ram
	* 0x2000 - 0x3FFF Rom Bank number
	* 0x4000 - 0x5FFF RAM bank number/Upper 2 bits of ROM bank number
	* 0x6000 - 0x7FFF Banking mode
	*/
	void Mbc1::Write(word address, byte value) {
		switch (address & 0xE000)
		{
		case 0x0000: {
			if (m_numrambanks == 0)
				return;

			m_enableRam = (value & 0x0F) == 0x0A;
		}//RAM register
			//any value with the lower 4 bits that are equal to 0x0A enables the ram
				   break;

		case 0x2000: //ROM bank 
		{
			/*
			* In theory it is impossible to set this register
			* to the value 0x00. But all the 5 bits of the register
			* are all compared to 0, so the programmer can set
			* the higher 2 bits to a value different from 0
			* and the expression REG == 0x00 returns false.
			* BUT, the mapper uses only the lower 3 bits
			* for the bank selection, so, even if the
			* register's value is differemt from
			* 0, the selected bank will be 0x00,
			* which is also mapped in region 0x0000 - 0x3FFF,
			* resulting in both memory ranges to have
			* the same mapped ROM
			*/

			//the register is only 5 bits
			m_second_bank_num = value & 0b11111;

			//register cannot be == 0x00
			if (m_second_bank_num == 0x00)
				m_second_bank_num++;

			//mask the register with the max index
			m_second_bank_num &= m_numbanks - 1;
		}
		break;

		case 0x4000: //ram bank number or upper bits of ram bank
		{
			//only 2 bits
			m_bank_num_hi = value & 0b11;
		}
		break;

		case 0x6000: //banking mode
		{
			if (m_numrambanks < 4) {
				LOG_ERR(m_state->GetLogger(), " Cannot switch mode, ram too small\n");
				return;
			}

			m_bankingMode = value & 0x1;
		}
		break;

		case 0xA000: {
			if (!m_enableRam)
				return;

			byte rambankSelect = 0;

			if (m_bankingMode) {
				rambankSelect = m_second_bank_num
					& (m_numrambanks - 1);
			}

			m_sram[(address - 0xA000) +
				(rambankSelect * 0x2000)] = value;
		} break;

		default:
			LOG_ERR(m_state->GetLogger(), "Writing to inacessible address {2:x}\n", address);
			break;
		}
	}

	byte Mbc1::GetCurrentBank(word at) const {
		if (at <= 0x3FFF) {
			byte bank = 0;

			if (m_bankingMode) {
				bank = (m_bank_num_hi << 5) & (m_numbanks - 1);
			}

			return bank;
		}

		return ((m_bank_num_hi << 5) + m_second_bank_num) & (m_numbanks - 1);
	}

	bool Mbc1::SupportsSaves() const {
		return m_numrambanks != 0;
	}

	const byte* Mbc1::GetRamBuffer() const {
		return m_sram;
	}

	void Mbc1::LoadRamSave(byte* buf) {
		std::copy_n(buf, m_numrambanks * 0x2000, m_sram);
	}

	Mbc1::~Mbc1() {
		delete[] m_therom;
		delete[] m_sram;
	}

	std::size_t Mbc1::DumpState(byte* buffer, std::size_t offset) {
		buffer[offset] = MemoryCard::GetType();
		buffer[offset + 1] = m_second_bank_num;
		buffer[offset + 2] = m_bank_num_hi;
		buffer[offset + 3] = m_first_bank_num;
		buffer[offset + 4] = m_enableRam;
		buffer[offset + 5] = m_bankingMode;
		buffer[offset + 6] = m_rambank;

		offset += 7;

		uint64_t sizekb = getRamKb(MemoryCard::GetRamSize());

		std::copy_n(m_sram, (uint64_t)sizekb * 1024, buffer + offset);

		return offset + (sizekb * 1024);
	}

	std::size_t Mbc1::LoadState(byte* buffer, std::size_t offset) {
		byte type = buffer[offset];

		if (type != MemoryCard::GetType()) {
			throw std::runtime_error("Invalid cartridge type");
		}

		m_second_bank_num = buffer[offset + 1];
		m_bank_num_hi = buffer[offset + 2];
		m_first_bank_num = buffer[offset + 3];
		m_enableRam = buffer[offset + 4];
		m_bankingMode = buffer[offset + 5];
		m_rambank = buffer[offset + 6];

		offset += 7;

		uint64_t sizekb = getRamKb(MemoryCard::GetRamSize());

		std::copy_n(buffer + offset, (uint64_t)sizekb * 1024, m_sram);

		return offset + (sizekb * 1024);
	}

	std::vector<Mbc1::replace_type> Mbc1::ApplyPatch(byte replace, word address, short compare) {
		std::vector<Mbc1::replace_type> list;

		if (address < 0x4000) {
			//Replace values in all the
			//possible banks that can
			//be mapped in region
			//0x0000 - 0x3FFF
			static constexpr uint8_t accessible_banks[] = {
				0x00, 0x20, 0x40, 0x60
			};

			uint8_t index = 0;

			while (index < 4 && accessible_banks[index] <= m_numbanks) {
				uint64_t bank_index = accessible_banks[index];

				uint64_t pos = (bank_index * 0x4000) + address;

				if (compare == -1 || m_therom[pos] == compare) {
					byte old = m_therom[pos];

					m_therom[pos] = replace;

					list.push_back(replace_type((word)bank_index, old));
				}

				index++;
			}
		}
		else {
			for (uint8_t bank = 1; bank < m_numbanks; bank++) {
				if (bank == 0x20 || bank == 0x40 || bank == 0x60) {
					continue;
				}

				uint64_t pos = ((uint64_t)bank * 0x4000) + (
					(uint64_t)address - 0x4000);

				if (compare == -1 || m_therom[pos] == compare) {
					byte old = m_therom[pos];

					m_therom[pos] = replace;

					list.push_back(replace_type((word)bank, old));
				}
			}
		}

		return list;
	}

	void Mbc1::RemovePatch(std::vector<Mbc1::replace_type> const& replaces, word address) {
		for (auto const& replace : replaces) {
			byte old_value = replace.second;
			word banknum = replace.first;

			address = address > 0x3FFF ? address - 0x4000 : address;

			uint64_t pos = ((uint64_t)banknum * 0x4000) + address;

			m_therom[pos] = old_value;
		}
	}

	byte Mbc1::ApplyShark(Cheats::GameShark const& shark) {
		if (shark.bank_number > m_numbanks) {
			return 0xFF;
		}

		uint64_t address = ((uint64_t)shark.address - 0xA000) +
			((uint64_t)shark.bank_number * 0x2000);

		byte old = m_sram[address];

		m_sram[address] = shark.new_data;

		return old;
	}
}