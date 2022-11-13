#pragma once

#include "../../common/Common.h"
#include "../output/OutputDevice.h"
#include "Channel.h"

#include <array>

namespace GameboyEmu::State {
	class EmulatorState;
}

namespace GameboyEmu::Sound {
	enum class Panning {
		mute = 0,
		left = 1,
		right = 2,
		middle = 3
	};

	class APU {
	public :
		APU(State::EmulatorState* state, OutputDevice* outdev);

		void WriteReg(word address, byte value);
		byte ReadReg(word address);

		void Tick(byte cycles);

		std::size_t DumpState(byte* buffer, std::size_t offset);
		std::size_t LoadState(byte* buffer, std::size_t offset);

		~APU();

	private :
		void mix_samples(std::array<short, 4> const& samples);

		float high_pass(float in);

	private :
		State::EmulatorState* m_state;
		OutputDevice* m_dev;

		AudioChannel* m_ch1;
		AudioChannel* m_ch2;
		AudioChannel* m_ch3;
		AudioChannel* m_ch4;

		byte m_left_vol;
		byte m_right_vol;

		bool m_enabled;

		Panning m_panning[4];

		byte* m_samples;
		word m_num_samples;

		word m_curr_cycles;

		float m_capacitor;

		static constexpr unsigned num_samples = 4096;
		static constexpr unsigned sample_rate = 44100;
		static constexpr unsigned cpu_clock = 4194304;
		static constexpr unsigned sample_clocks = cpu_clock / sample_rate;
	};
}