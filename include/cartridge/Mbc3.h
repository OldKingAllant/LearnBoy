#pragma once

#include "MemoryCard.h"
#include "../timing/RealTimeClock.h"

namespace GameboyEmu {
	namespace State {
		class EmulatorState;
	}
}

namespace GameboyEmu::Cartridge {

	class Mbc3 : public MemoryCard {
	public :
		Mbc3(State::EmulatorState* state, byte* data, unsigned numb);

		byte Read(word address) override;
		void Write(word address, byte value) override;

		byte GetCurrentBank(word at) const override;

		bool SupportsSaves() const override;
		const byte* GetRamBuffer() const override;
		void LoadRamSave(byte* buf) override;

		std::size_t DumpState(byte* buffer, std::size_t offset) override;
		std::size_t LoadState(byte* buffer, std::size_t offset) override;

		std::vector<replace_type> ApplyPatch(byte replace, word address, short compare) override;
		void RemovePatch(std::vector<replace_type> const& replaces, word address) override;
		
		byte ApplyShark(Cheats::GameShark const& shark) override;

		~Mbc3() override;

	private :
		State::EmulatorState* m_state;
		byte* m_rom;

		byte m_bank_number;

		byte* m_sram;
		byte m_ram_bank_number;

		bool m_enable_rtc_ram;

		byte m_total_banks;
		byte m_total_ram_banks;

		byte m_rtc_reg_select;

		bool m_rtc_or_ram;

		//Place RTC Register here
		Timing::RealTimeClock m_rtc;
	};
}