#include "../../../include/sound/apu/WaveChannel.h"

namespace GameboyEmu::Sound {
	WaveChannel::WaveChannel() :
		m_len(), m_volume(), m_wavelen(),
		m_output(), m_dac(false),
		m_curr_sample(),
		m_counter(255), m_seq(),
		m_waveram{} {}

	void WaveChannel::WriteFreq(byte value) {
		m_wavelen = (m_wavelen & 0b11100000000) |
			value;
	}

	void WaveChannel::WriteControl(byte value) {
		byte high = value & 0b111;

		m_wavelen = (high << 8) | (m_wavelen & 0xFF);

		m_selection = GET_BIT(value, 6);

		byte restart = GET_BIT(value, 7);

		if (restart) {
			Restart();
		}
	}

	byte WaveChannel::ReadFreq() const {
		return 0xFF;
	}

	byte WaveChannel::ReadControl() const {
		return m_selection << 6;
	}

	bool WaveChannel::Clock() {
		if (m_enabled && m_dac) {
			m_seq.Clock(this);

			bool advance = m_timer.Clock(this);

			if (advance) {
				m_curr_sample++;

				if (m_curr_sample >= 32) {
					m_curr_sample = 0;
				}

				if (m_curr_sample % 2 == 0) {
					m_output = m_waveram[m_curr_sample / 2] & 0xF;
				}
				else {
					m_output = m_waveram[m_curr_sample / 2] >> 4;
				}

				switch (m_volume)
				{
				case 0b00:
					m_output = 0;
					break;

				case 0b01:
					break;

				case 0b10:
					m_output >>= 1;
					break;

				case 0b11:
					m_output >>= 2;
					break;

				default:
					break;
				}
			}
		}
		return false;
	}

	void WaveChannel::Disable() {
		m_enabled = false;
	}

	void WaveChannel::Restart() {
		m_counter.Reload(m_len);

		m_timer.Reload(this);

		m_seq.Reset();

		m_enabled = m_dac;
	}

	LenCounter* WaveChannel::GetLenCounter() {
		return &m_counter;
	}

	byte WaveChannel::GetOutput() const {
		return m_output;
	}

	WaveChannel::~WaveChannel() {
		//
	}

	word WaveChannel::GetCalculatedPeriod() const {
		return (2048 - m_wavelen) * 2;
	}

	void WaveChannel::WriteLen(byte value) {
		m_len = value;
	}

	void WaveChannel::WriteOutLevel(byte value) {
		m_volume = (value >> 5) & 0b11;
	}

	byte WaveChannel::ReadOutLevel() const {
		return m_volume << 5;
	}

	void WaveChannel::WriteWaveRam(byte offset, byte value) {
		m_waveram[offset] = value;
	}

	byte WaveChannel::ReadWaveRam(byte offset) {
		return m_waveram[offset];
	}

	void WaveChannel::SetEnabled(byte en) {
		m_dac = (bool)(GET_BIT(en, 7));
	}
}