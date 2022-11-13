#include "../../../include/sound/apu/PulseChannel.h"

namespace GameboyEmu::Sound {
	PulseChannel::PulseChannel(bool use_sweep) :
		m_duty_offset(), m_duty_id(),
		m_dac(false),
		m_envelope(), m_counter(64),
		m_sweep(nullptr), m_seq(),
		m_output(), m_len()
	{
		if (use_sweep) {
			m_sweep = new Sweep();
		}
	}

	void PulseChannel::WriteFreq(byte value) {
		m_freq = (m_freq & 0b11100000000) | value;
	}

	void PulseChannel::WriteControl(byte value) {
		byte restart = GET_BIT(value, 7);

		m_selection = GET_BIT(value, 6);

		m_freq = ((value & 0b111) << 8) | 
			(m_freq & 0xFF);

		if (restart) {
			Restart();
		}
	}

	byte PulseChannel::ReadFreq() const {
		return (byte)(m_freq & 0xFF);
	}

	byte PulseChannel::ReadControl() const {
		return m_selection << 6;
	}

	byte PulseChannel::ReadDuty() const {
		return m_duty_id << 6;
	}

	void PulseChannel::WriteDuty(byte val) {
		byte duty = (val >> 6) & 0b11;

		m_duty_id = duty;
		m_duty_offset = 0;

		byte len = (val & 0b111111);

		m_len = len;
	}

	byte PulseChannel::ReadEnvelope() const {
		return m_envelope.Read();
	}

	void PulseChannel::WriteEnvelope(byte value) {
		if ((value & 0xF8) != 0)
			m_dac = true;
		else
			m_dac = false;

		m_envelope.Write(value);
	}

	byte PulseChannel::ReadSweep() const {
		if (!m_sweep)
			return 0xFF;

		return m_sweep->Read();
	}

	void PulseChannel::WriteSweep(byte value) {
		if (!m_sweep)
			return;

		m_sweep->Write(value);
	}

	bool PulseChannel::Clock() {
		if (m_enabled && m_dac) {
			m_seq.Clock(this);

			bool change = m_timer.Clock(this);

			m_output = (wave_duty[m_duty_id][m_duty_offset] & 1) *
				m_envelope.GetVolume();

			if (change) {
				m_duty_offset = (m_duty_offset + 1) % 8;

				return true;
			}
		}

		return false;
	}

	void PulseChannel::Disable() {
		m_enabled = false;
	}

	void PulseChannel::Restart() {
		m_envelope.Reload(this);

		if (m_sweep)
			m_sweep->Reload(this);

		m_timer.Reload(this);
		
		m_counter.Reload(m_len);

		m_seq.Reset();

		m_enabled = true;
	}

	LenCounter* PulseChannel::GetLenCounter() {
		return &m_counter;
	}

	Sweep* PulseChannel::GetSweep() {
		return m_sweep;
	}

	Envelope* PulseChannel::GetEnvelope() {
		return &m_envelope;
	}

	byte PulseChannel::GetOutput() const {
		return m_enabled ? m_output : 0;
	}

	PulseChannel::~PulseChannel() {
		if (m_sweep)
			delete m_sweep;
	}

	word PulseChannel::GetCalculatedPeriod() const {
		return (2048 - m_freq) * 4;
	}
}