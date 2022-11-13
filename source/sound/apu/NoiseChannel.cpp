#include "../../../include/sound/apu/NoiseChannel.h"

namespace GameboyEmu::Sound {
	NoiseChannel::NoiseChannel() :
		m_envelope(), m_counter(64),
		m_seq(), m_len(),
		m_shift_freq(), m_step(),
		m_ratio(), m_output(), 
		m_lfsr(0x7FFF)
	{}

	void NoiseChannel::WriteFreq(byte value) {}

	byte NoiseChannel::ReadFreq() const {
		return 0xFF;
	}

	byte NoiseChannel::ReadLen() const {
		return 0xFF;
	}

	void NoiseChannel::WriteLen(byte value) {
		m_len = value;
	}

	void NoiseChannel::WriteControl(byte value) {
		m_selection = GET_BIT(value, 6);

		byte restart = GET_BIT(value, 7);

		if (restart) {
			Restart();
		}
	}

	byte NoiseChannel::ReadControl() const {
		return (m_selection << 6);
	}

	void NoiseChannel::WritePolynomialCounter(byte value) {
		m_shift_freq = (value >> 4) & 0b1111;
		m_step = GET_BIT(value, 3);
		m_ratio = value & 0b111;
	}

	byte NoiseChannel::ReadPolynomialCounter() const {
		return (m_shift_freq << 4) |
			(m_step << 3) |
			m_ratio;
	}

	byte NoiseChannel::ReadEnvelope() const {
		return m_envelope.Read();
	}

	void NoiseChannel::WriteEnvelope(byte value) {
		m_envelope.Write(value);
	}

	void NoiseChannel::clock_lfsr() {
		byte bitlow = m_lfsr & 1;
		byte bit1 = GET_BIT(m_lfsr, 1);

		byte xored = bitlow ^ bit1;

		m_lfsr >>= 1;

		m_lfsr |= (xored << 14);

		if (m_step) {
			m_lfsr &= ~0x40;

			m_lfsr |= (xored << 6);
		}
	}

	bool NoiseChannel::Clock() {
		if (m_enabled) {
			m_seq.Clock(this);

			bool advance = m_timer.Clock(this);

			if (advance) {
				clock_lfsr();

				m_output = (~m_lfsr & 1) *
					m_envelope.GetVolume();

				return true;
			}
		}

		return false;
	}

	void NoiseChannel::Disable() {
		m_enabled = false;
	}

	void NoiseChannel::Restart() {
		m_envelope.Reload(this);

		m_counter.Reload(m_len);

		m_timer.Reload(this);

		m_seq.Reset();

		m_enabled = true;
	}

	LenCounter* NoiseChannel::GetLenCounter() {
		return &m_counter;
	}

	Envelope* NoiseChannel::GetEnvelope() {
		return &m_envelope;
	}

	NoiseChannel::~NoiseChannel() {}

	byte NoiseChannel::GetOutput() const {
		return m_output;
	}

	word NoiseChannel::GetCalculatedPeriod() const {
		word divider = m_ratio == 0 ? 8 : 16 * m_ratio;

		unsigned res = (divider << m_shift_freq);

		//_ASSERT(res < 0xFFFF);

		return res;
	}
}