#include "../../../include/sound/apu/Channel.h"

namespace GameboyEmu::Sound {
	AudioChannel::AudioChannel() :
		m_freq(), m_counter_select(), 
		m_enabled(false), m_selection()
	{}

	bool AudioChannel::Enabled() const {
		return m_enabled;
	}

	word AudioChannel::GetFrequency() const {
		return m_freq;
	}

	byte AudioChannel::GetSelection() const {
		return m_selection;
	}

	void AudioChannel::SetFrequency(word freq) {
		m_freq = freq;
	}

	AudioChannel::~AudioChannel() {}
}