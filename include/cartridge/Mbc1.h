#pragma once

#include "MemoryCard.h"

namespace GameboyEmu::State {
	class EmulatorState;
}

namespace GameboyEmu::Cartridge {

	class Mbc1 : public MemoryCard {
	public:
		Mbc1(State::EmulatorState* state, byte* data, unsigned numb);

		byte Read(word address) override;
		void Write(word address, byte value) override;

		byte GetCurrentBank(word at) const override;
		
		bool SupportsSaves() const override;
		const byte* GetRamBuffer() const override;
		void LoadRamSave(byte* buf) override;

		~Mbc1();

		std::size_t DumpState(byte* buffer, std::size_t offset) override;
		std::size_t LoadState(byte* buffer, std::size_t offset) override;

		std::vector<replace_type> ApplyPatch(byte replace, word address, short compare) override;
		void RemovePatch(std::vector<replace_type> const& replaces, word address) override;

		byte ApplyShark(Cheats::GameShark const& shark) override;

	private:
		State::EmulatorState* m_state;
		byte* m_therom;     //rom memory
		unsigned m_numbytes; //len of rom
		byte m_second_bank_num;     //current rom bank
		byte m_bank_num_hi;   //high bits of rom bank number
		byte m_first_bank_num;
		bool m_enableRam;   //ram enabled
		bool m_bankingMode; //banking mode 0/1
		unsigned m_numbanks;//number of rom banks
		byte m_rambank;     //current ram bank
		byte m_numrambanks; //number of ram banks

		byte* m_sram;
	};

}