#pragma once

#include "../../common/Common.h"

namespace GameboyEmu::Sound {
	class AudioChannel;

	class Envelope {
	public:
		Envelope();

		byte Read() const;
		void Write(byte val);

		void Clock(AudioChannel* channel);

		void Reload(AudioChannel* channel);

		byte GetVolume() const;

	private:
		byte m_init_volume;
		byte m_direction;
		byte m_period;
		byte m_reg;
		byte m_counter;

		byte m_volume;
	};

	class Sweep {
	public :
		Sweep();

		byte Read() const;
		void Write(byte val);

		void Clock(AudioChannel* channel);

		void Reload(AudioChannel* channel);

	private :
		byte m_reg;
		byte m_direction;
		byte m_period;
		byte m_shifts;

		byte m_clocks;

		bool m_enabled;

		word m_shadow;
	};
}