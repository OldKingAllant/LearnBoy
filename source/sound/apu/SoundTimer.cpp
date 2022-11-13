#include "../../../include/sound/apu/SoundTimer.h"
#include "../../../include/sound/apu/Channel.h"

namespace GameboyEmu::Sound {
	SoundTimer::SoundTimer() :
		m_period(), m_cycles()
	{}

	bool SoundTimer::Clock(AudioChannel* channel) {
		m_cycles++;

		if (m_cycles >= m_period) {
			m_cycles = 0;

			m_period = channel->GetCalculatedPeriod();

			return true;
		}

		return false;
	}

	void SoundTimer::Reload(AudioChannel* channel) {
		m_period = channel->GetCalculatedPeriod();
		m_cycles = 0;
	}
}