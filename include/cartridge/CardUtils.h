#pragma once

/*
* File provides an API to functions
* that are used to extract header 
* informations from a given std::span
*/

/*
*
* Memory card address range:
* 0x0000 - 0x3FFF -> first bank
* 0x4000 - 0x7FFF -> 01-N Memory bank
*
* The header is located in memory
* range 0x0100 - 0x014F
*
* Header layout :
* 0x0100 - 0x0103 = entry point
* 0x0104 - 0x0133 = nintendo logo (anti-piracy test)
* 0x0134 - 0x0143 = Game title (upper case), size could
*   change depending on cartridge version
*   alternative sizes are 15 or 11 bytes
* 0x013F - 0x0142 = (in newer cartridges this is the
*   manufacturer code)
* 0x0143 = CGB flag, if the cartridge is
*   used in CGB mode (0x80 or 0xC0)
* 0x0144 - 0x0145 = new license code, useful
*   if the Old License Code is 0x33
* 0x0146 = support more SGB functions
* 0x0147 = cartridge type
* 0x0148 = rom size kB (32 * 1 << value)
* 0x0149 = RAM size
* 0x014A = destination code (0x00, 0x01)
* 0x014B = old license code
* 0x014C = rom version
* 0x014D = checksum
* 0x014E - 0x014F = global checksum
*
*
*/

#include "../common/Common.h"
#include <span>
#include <optional>

#include <string_view>

//Header starts at ROM address 0x100
#define HEADER_START 0x100

namespace GameboyEmu::Cartridge {
		/*
		* 0x80 - Supports CGB functions
		* 0xC0 - Only works on CGB
		* 0x00 - Probably DMG only
		*/
		enum class CGB_Flag {
			support = 0x80,
			cgb_only = 0xC0,
			unknown = 0x00
		};

		/*
		* A bunch of new licensee codes
		*/
		enum class Licensee_Code {
			none = 0x00,
			nintendo_r_d1 = 0x01,
			capcom = 0x08,
			ea = 0x013,
			hudson_soft = 0x018,
			b_ai = 0x019,
			kss = 0x20,
			pow = 0x22,
			pcm_complete = 0x24,
			san_x = 0x25,
			kemco_japan = 0x28,
			seta = 0x29,
			viacom = 0x30,
			nintendo = 0x31,
			bandai = 0x32,
			ocean_acclaim = 0x33,
			konami = 0x34,
			idk = 0xFF
		};

		/*
		* 0x146
		*
		* 0x03 - Game uses SGB functions
		* 0x00 (or anything else) - SGB functions disabled
		*/
		enum class SGB_Flag {
			active = 0x03,
			disabled = 0x00
		};

		/*
		* Supported types
		*
		* 0x147
		*
		* 0x00 - Rom only, no controller
		* 0x01 - MBC1
		* 0x02 - MBC1 + RAM
		* 0x03 - MBC1 + RAM + BATTERY
		* ...
		*/
		enum class Type {
			rom_only = 0x00,
			mcb1 = 0x01,
			mbc1_ram = 0x02,
			mbc1_ram_battery = 0x03
		};

		/*
		* 0x148 Rom size
		*
		* 0x00 - 32 KB
		* 0x01 - 64 KB
		*/
		enum class ROM_Size {
			kb32 = 0x00,
			kb64 = 0x01
		};

		/*
		* 0x149
		*
		* 0x00 - No Ram
		* 0x01 - Unused
		* 0x02 - 8 KB
		*/
		enum class RAM_Size {
			no_ram = 0x00,
			unused = 0x01,
			bank1 = 0x02
		};

		/*
		* 0x14A
		*
		* Destionation code
		*
		* 0x00 - Shipped to Japan
		* 0x01 - Only overseas
		*/
		enum class Dest_Code {
			japan = 0x00,
			other = 0x01
		};

		/*
		* Converts CGB_Flag id to string representation
		*
		* @param fl - The flag value
		*/
		std::string_view GetCGBFlagString(CGB_Flag fl);

		/*
		* Returns licensee code id as string
		*
		* @param code - The code
		*/
		std::string_view GetLicenseeCodeString(Licensee_Code code);

		/*
		* Returns SGB flag as string
		*
		* @param code - The code
		*/
		std::string_view GetSGBFlagAsString(SGB_Flag fl);

		/*
		* Returns type as string
		*
		* @param tp - The type id
		*/
		std::string_view GetCartridgeTypeAsString(Type tp);

		/*
		* Returns ROM size as string
		*
		* @param sz - Size id
		*/
		std::string_view GetRomSizeAsString(ROM_Size sz);

		/*
		* Returns RAM size as string
		*
		* @param sz - The size id
		*/
		std::string_view GetRamSizeAsString(RAM_Size sz);

		/*
		* Returns destination code as string
		*
		* @param dest - The code
		*/
		std::string_view GetDestCodeAsString(Dest_Code dest);

		//Retrieves old license code from the header
		byte GetOldLicenseeCode(span const& header);

		//Retrieves title from header (16 or 11 bytes)
		span GetTitle(span const& header);

		//Retrieves manufacturer code (if present)
		std::optional<span> GetManufacturer(span const& header);

		//Retrieves CGB flag, if present
		std::optional<byte> GetCGBFlag(span const& header);

		//Retrieves new licensee code, if present
		std::optional<std::pair<byte, byte>> GetNewLicenseeCode(span const& header);

		//Retrieves cartridge type
		byte GetCartridgeType(span const& header);

		//Rom size
		byte GetROMSize(span const& header);

		//Ram size
		byte GetRAMSize(span const& header);

		//Destination code
		byte GetDestCode(span const& header);

		//Rom version
		byte GetROMVersion(span const& header);

		//Header checksum
		byte GetChecksum(span const& header);

		//Global checksum
		std::pair<byte, byte> GetGlobalChecksum(span const& header);
}