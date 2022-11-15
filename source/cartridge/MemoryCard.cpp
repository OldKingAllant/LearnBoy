#include "../../include/cartridge/MemoryCard.h"

#include "../../include/cartridge/CardUtils.h"
#include <fmt/format.h>

namespace GameboyEmu {
	namespace Cartridge {

		word getRamKb(byte ramSz) {
			switch (ramSz)
			{
			case 0x00:
			case 0x01:
				return 0;

			case 0x02:
				return 8;

			case 0x03:
				return 32;

			case 0x04:
				return 128;

			case 0x05:
				return 64;

			default:
				break;
			}

			return 0;
		}

		MemoryCard::MemoryCard(span const& header) {
			auto titleSpan = GameboyEmu::Cartridge::GetTitle(header);
			m_title = std::string(titleSpan.begin(), titleSpan.end());

			m_manufacturer = GameboyEmu::Cartridge::GetManufacturer(header);
			m_CGBflag = GameboyEmu::Cartridge::GetCGBFlag(header);
			m_licenseeCodeOld = GameboyEmu::Cartridge::GetOldLicenseeCode(header);

			auto newlicensee = GameboyEmu::Cartridge::GetNewLicenseeCode(header);

			if (newlicensee.has_value()) {
				auto p = newlicensee.value();

				m_newLicenseeCode = (p.second << 8) |
					p.first;
			}

			m_type = GameboyEmu::Cartridge::GetCartridgeType(header);
			m_romSize = GameboyEmu::Cartridge::GetROMSize(header);
			m_ramSize = GameboyEmu::Cartridge::GetRAMSize(header);
			m_destCode = GameboyEmu::Cartridge::GetDestCode(header);
			m_romVersion = GameboyEmu::Cartridge::GetROMVersion(header);
			m_headerChecksum = GameboyEmu::Cartridge::GetChecksum(header);

			auto global = GameboyEmu::Cartridge::GetGlobalChecksum(header);

			m_globalChecksum =
				(global.second << 8) | global.first;

			m_calculatedChecksum = 0;

			for (word address = 0x0134; address < 0x014D; address++) {
				m_calculatedChecksum = m_headerChecksum + header[address - HEADER_START];
			}
		}

		std::optional<word> MemoryCard::GetLicensee() const {
			return m_newLicenseeCode;
		}

		std::string_view MemoryCard::GetTitle() const {
			return std::string_view(m_title);
		}

		std::optional<span> MemoryCard::GetManufacturer() const {
			return m_manufacturer;
		}

		::std::optional<byte> MemoryCard::GetCGBFlag() const {
			return m_CGBflag;
		}

		byte MemoryCard::GetOldLicensee() const {
			return m_licenseeCodeOld;
		}

		byte MemoryCard::GetType() const {
			return m_type;
		}

		byte MemoryCard::GetRomSize() const {
			return m_romSize;
		}

		byte MemoryCard::GetRamSize() const {
			return m_ramSize;
		}


		byte MemoryCard::GetDestCode() const {
			return m_destCode;
		}

		byte MemoryCard::GetVersion() const {
			return m_romVersion;
		}

		byte MemoryCard::GetHeaderChecksum() const {
			return m_headerChecksum;
		}

		word MemoryCard::GetGlobalChecksum() const {
			return m_globalChecksum;
		}

		std::string MemoryCard::Dump() const {
			std::string manufacturer = "NULL";

			if (m_manufacturer.has_value()) {
				auto const& manSpan = m_manufacturer.value();

				manufacturer = fmt::format(
					"{:x} {:x} {:x} {:x}",
					manSpan[0], manSpan[1],
					manSpan[2], manSpan[3]
				);
			}

			return fmt::format(
				"Title : {0}\nManufacturer : {1}\n"
				"CGB Flag : {2:x}\nNew licensee code : {3:x}\n"
				"Type : {4:x}\nRom size : {5:x}\n"
				"Ram Size : {6:x}\nDest code : {7:x}\n"
				"Rom version : {8:x}\nHeader checksum : {9:x}\n"
				"Global checksum : {10:x}\nOld licensee : {11:x}",
				m_title,
				manufacturer,
				m_CGBflag.value_or((byte)0x00),
				m_newLicenseeCode.value_or((word)0x00),
				m_type,
				m_romSize,
				m_ramSize,
				m_destCode,
				m_romVersion,
				m_headerChecksum,
				m_globalChecksum,
				m_licenseeCodeOld
			);
		}

		byte MemoryCard::GetCalculatedHeaderChecksum() const {
			return m_calculatedChecksum;
		}

		MemoryCard::~MemoryCard() {}
	}
}