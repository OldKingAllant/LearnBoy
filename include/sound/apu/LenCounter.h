#pragma once

#include "../../common/Common.h" 

namespace GameboyEmu::Sound {
	class AudioChannel;

	class LenCounter {
	public :
		LenCounter(word max);

		void Clock(AudioChannel* channel);
		void Reload(word len);

	private :
		word m_len;
		word m_max_len;
	};
}