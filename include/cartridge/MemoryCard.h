#pragma once

/*
* File containing the abstract class
* representing a cartridge. 
* 
* Contains the header informations
* and provides virtual methods
* for reading from and writing
* to the cartridge.
* 
* It provides getters for retrieving
* infos
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

#include <optional>
#include <string_view>
#include <vector>
#include <string>

#include "../cheats/GameShark.h"

namespace GameboyEmu {
	namespace Cartridge {

		word getRamKb(byte ramSz);

		class MemoryCard {
		public:
			MemoryCard(span const& header);

			virtual byte Read(word address) = 0;
			virtual void Write(word address, byte value) = 0;
			
			std::optional<word> GetLicensee() const;
			std::string_view GetTitle() const;
			std::optional<span> GetManufacturer() const;
			std::optional<byte> GetCGBFlag() const;
			byte GetOldLicensee() const;
			byte GetType() const;
			byte GetRomSize() const;
			byte GetRamSize() const;
			byte GetDestCode() const;
			byte GetVersion() const;
			byte GetHeaderChecksum() const;
			word GetGlobalChecksum() const;

			std::string Dump() const;

			byte GetCalculatedHeaderChecksum() const;

			virtual byte GetCurrentBank(word at) const = 0;

			virtual ~MemoryCard();

			virtual bool SupportsSaves() const = 0;
			virtual const byte* GetRamBuffer() const = 0;
			virtual void LoadRamSave(byte* buf) = 0;

			virtual std::size_t DumpState(byte* buffer, std::size_t offset) = 0;
			virtual std::size_t LoadState(byte* buffer, std::size_t offset) = 0;

			using replace_type = std::pair<word, byte>;

			virtual std::vector<replace_type> ApplyPatch(byte replace, word address, short compare) = 0;
			virtual void RemovePatch(std::vector<replace_type> const& replaces, word address) = 0;

			virtual byte ApplyShark(Cheats::GameShark const& shark) = 0;

		private:
			byte m_licenseeCodeOld;
			std::string m_title;
			std::optional<span> m_manufacturer;
			std::optional<byte> m_CGBflag;
			std::optional<word> m_newLicenseeCode;
			byte m_type;
			byte m_romSize;
			byte m_ramSize;
			byte m_destCode;
			byte m_romVersion;
			byte m_headerChecksum;
			word m_globalChecksum;

			byte m_calculatedChecksum;
		};
	}
}