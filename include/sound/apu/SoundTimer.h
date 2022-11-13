#pragma once

#include "../../common/Common.h"

namespace GameboyEmu::Sound {
	class AudioChannel;

	class SoundTimer {
	public :
		SoundTimer();

		bool Clock(AudioChannel* channel);

		void Reload(AudioChannel* channel);

	private :
		unsigned m_period;
		unsigned m_cycles;
	};
}