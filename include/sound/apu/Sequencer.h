#pragma once

#include "../../common/Common.h"

namespace GameboyEmu::Sound {
	class AudioChannel;

	class PulseChannel;
	class NoiseChannel;
	class WaveChannel;

	template <typename ChType>
	class Sequencer {};

	template <>
	class Sequencer<PulseChannel> {
	public :
		Sequencer();

		void Clock(PulseChannel* channel);

		void Reset();

		static constexpr word cycles_per_step = 4194304 / 512;

	private :
		word m_cycles;
		byte m_step;
	};

	template <>
	class Sequencer<NoiseChannel> {
	public:
		Sequencer();

		void Clock(NoiseChannel* channel);

		void Reset();

		static constexpr word cycles_per_step = 4194304 / 512;

	private:
		word m_cycles;
		byte m_step;
	};

	template <>
	class Sequencer<WaveChannel> {
	public:
		Sequencer();

		void Clock(WaveChannel* channel);

		void Reset();

		static constexpr word cycles_per_step = 4194304 / 512;

	private:
		word m_cycles;
		byte m_step;
	};
}