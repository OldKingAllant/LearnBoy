#include "../../include/cartridge/CardUtils.h"

#include <span>
#include <optional>

namespace GameboyEmu {

	namespace Cartridge {

		std::string_view GetCGBFlagString(CGB_Flag fl) {
			switch (fl)
			{
			case CGB_Flag::support:
				return "Supported";
				break;
			case CGB_Flag::cgb_only:
				return "CGB Only";
				break;
			case CGB_Flag::unknown:
				return "Unknown";
				break;
			default:
				return "Unknown";
				break;
			}
		}

		std::string_view GetLicenseeCodeString(Licensee_Code code) {
			switch (code)
			{
			case Licensee_Code::none:
				return "None";
				break;
			case Licensee_Code::nintendo_r_d1:
				return "Nintendo R&D1";
				break;
			case Licensee_Code::capcom:
				return "Capcom";
				break;
			case Licensee_Code::ea:
				return "Electronic Arts";
				break;
			case Licensee_Code::hudson_soft:
				return "Hudson Soft";
				break;
			case Licensee_Code::b_ai:
				return "B-ai";
				break;
			case Licensee_Code::kss:
				return "KSS";
				break;
			case Licensee_Code::pow:
				return "POW";
				break;
			case Licensee_Code::pcm_complete:
				return "PCM Complete";
				break;
			case Licensee_Code::san_x:
				return "san-x";
				break;
			case Licensee_Code::kemco_japan:
				return "Kemco Japan";
				break;
			case Licensee_Code::seta:
				return "Seta";
				break;
			case Licensee_Code::viacom:
				return "Viacom";
				break;
			case Licensee_Code::nintendo:
				return "Nintendo";
				break;
			case Licensee_Code::bandai:
				return "Bandai Namco";
				break;
			case Licensee_Code::ocean_acclaim:
				return "Ocean/Acclaim";
				break;
			case Licensee_Code::konami:
				return "Konami";
				break;
			case Licensee_Code::idk:
				return "Unknown";
				break;
			default:
				return "Unknown";
				break;
			}
		}

		std::string_view GetSGBFlagAsString(SGB_Flag fl) {
			switch (fl)
			{
			case SGB_Flag::active:
				return "Active";
				break;
			case SGB_Flag::disabled:
				return "Disabled";
				break;
			default:
				return "Disabled";
				break;
			}
		}

		std::string_view GetCartridgeTypeAsString(Type tp) {
			switch (tp)
			{
			case Type::rom_only:
				return "ROM";
				break;
			case Type::mcb1:
				return "MBC1";
				break;
			case Type::mbc1_ram:
				return "MBC1+RAM";
				break;
			case Type::mbc1_ram_battery:
				return "MBC1+RAM+BATTERY";
				break;
			default:
				return "Invalid";
				break;
			}
		}

		std::string_view GetRomSizeAsString(ROM_Size sz) {
			switch (sz)
			{
			case ROM_Size::kb32:
				return "32 KB";
				break;
			case ROM_Size::kb64:
				return "64 KB";
				break;
			default:
				return "Invalid";
				break;
			}
		}

		std::string_view GetRamSizeAsString(RAM_Size sz) {
			switch (sz)
			{
			case RAM_Size::no_ram:
				return "No RAM";
				break;
			case RAM_Size::unused:
				return "UNUSED";
				break;
			case RAM_Size::bank1:
				return "8 KB";
				break;
			default:
				return "No RAM";
				break;
			}
		}

		std::string_view GetDestCodeAsString(Dest_Code dest) {
			switch (dest)
			{
			case Dest_Code::japan:
				return "Japan";
				break;
			case Dest_Code::other:
				return "Overseas";
				break;
			default:
				return "Overseas";
				break;
			}
		}

		/*
		* If the old licensee code is equal to
		* 0x33, the title is shorter and new
		* flags are added.
		*
		* New data :
		* - Manufacturer code
		* - CGB flag
		* - New licensee code
		*
		*/

		byte GetOldLicenseeCode(span const& header) {
			return header[0x014B - HEADER_START];
		}

		/*
		* Len of 16 or 11 depending on old licensee code,
		* all characters are upper case ASCII
		*/
		span GetTitle(span const& header) {
			unsigned begin = 0x0134 - HEADER_START;
			unsigned len = 0x0143 - HEADER_START - begin;

			//if licensee code old == 0x33, then the title's len is 11 bytes
			if (GetOldLicenseeCode(header) == 0x33)
				len = 0x013F - HEADER_START - begin;

			return span(header.begin() + begin, header.begin() + begin + len);
		}

		/*
		* A 4 bytes long code, mapping
		* to a manufacturer name
		*/
		std::optional<span> GetManufacturer(span const& header) {
			if (GetOldLicenseeCode(header) != 0x33)
				return std::nullopt;

			unsigned beg = 0x013F - HEADER_START;


			return span(header.begin() + beg, header.begin() + beg + 4);
		}

		/*
		* The CGB flag states if the cartridge
		* uses CGB features
		*/
		std::optional<byte> GetCGBFlag(span const& header) {
			/*if (getOldLicenseeCode(header) != 0x33)
				return std::nullopt;*/

			return header[0x0143 - HEADER_START];
		}

		/*
		* The new licensee code
		*/
		std::optional<std::pair<byte, byte>> GetNewLicenseeCode(span const& header) {
			if (GetOldLicenseeCode(header) != 0x33)
				return std::nullopt;

			return std::pair(header[0x0144 - HEADER_START],
				header[0x0145 - HEADER_START]);
		}

		/*
		* Cartridge type :
		*
		* 0x00 - Rom only
		* 0x01 - MBC1
		* ...
		*/
		byte GetCartridgeType(span const& header) {
			return header[0x0147 - HEADER_START];
		}

		/*
		* Rom size, in blocks of
		* 32 KB.
		*
		* Calculate size :
		* 32 KB * (1 << size)
		*/

		byte GetROMSize(span const& header) {
			return header[0x0148 - HEADER_START];
		}

		/*
		* Ram size :
		*
		* 0x00 No ram
		* 0x01 Not used
		* 0x02 8 KB
		* 0x03 32 KB
		* 0x04 128 KB
		* 0x05 64 KB
		*/
		byte GetRAMSize(span const& header) {
			return header[0x0149 - HEADER_START];
		}

		/*
		* Destination code:
		*
		* 0x00 - Ship to Japan
		* 0x01 - Overseas
		*/
		byte GetDestCode(span const& header) {
			return header[0x014A - HEADER_START];
		}

		/*
		* Rom version number
		*/
		byte GetROMVersion(span const& header) {
			return header[0x014C - HEADER_START];
		}

		/*
		* Header checksum, used by the bootrom
		* to verify integrity of cartridge
		*/
		byte GetChecksum(span const& header) {
			return header[0x014D - HEADER_START];
		}

		/*
		* Global checksum of the cartridge
		*/
		std::pair<byte, byte> GetGlobalChecksum(span const& header) {
			return std::pair(
				header[0x014E - HEADER_START],
				header[0x014F - HEADER_START]);
		}
	}
}