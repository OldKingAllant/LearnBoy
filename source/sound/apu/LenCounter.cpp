#include "../../../include/sound/apu/LenCounter.h"
#include "../../../include/sound/apu/Channel.h"

namespace GameboyEmu::Sound {
	LenCounter::LenCounter(word max) :
		m_len(0), m_max_len(max)
	{}

	void LenCounter::Clock(AudioChannel* channel) {
		if (m_len) {
			m_len--;

			if (m_len == 0)
				channel->Disable();
		}
	}

	void LenCounter::Reload(word len) {
		m_len =  len == 0 ? m_max_len : len;
	}
}