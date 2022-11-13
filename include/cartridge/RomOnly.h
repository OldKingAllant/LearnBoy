#pragma once

#include "MemoryCard.h"

/*
* Derives the MemoryCard class 
* and implementes functionality 
* for a rom only cartridge
*/

namespace GameboyEmu {
	namespace State {
		class EmulatorState;
	}

	namespace Cartridge {

		class RomOnly : public MemoryCard {
		private:
			State::EmulatorState* m_state;
			byte* m_rom;
			unsigned m_bytes;

		public:
			RomOnly(State::EmulatorState* state, byte* theRom, unsigned numbytes);

			//Read and write generic cartridge
			byte Read(word address) override;
			void Write(word address, byte value) override;

			byte GetCurrentBank(word at) const override;

			//Dump header informations
			std::string DumpInfo() const;

			~RomOnly();

			bool SupportsSaves() const override;
			const byte* GetRamBuffer() const override;
			void LoadRamSave(byte* buf) override;

			std::size_t DumpState(byte* buffer, std::size_t offset) override;
			std::size_t LoadState(byte* buffer, std::size_t offset) override;

			std::vector<replace_type> ApplyPatch(byte replace, word address, short compare) override;
			void RemovePatch(std::vector<replace_type> const& replaces, word address) override;

			byte ApplyShark(Cheats::GameShark const& shark) override;
		};
	}
}