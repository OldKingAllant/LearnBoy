#include "../../../include/sound/apu/EnvelopeSweep.h"
#include "../../../include/sound/apu/Channel.h"

namespace GameboyEmu::Sound {
	Envelope::Envelope() :
		m_init_volume(), m_direction(),
		m_period(), m_reg(), m_counter(),
		m_volume()
	{}

	byte Envelope::Read() const {
		return m_reg;
	}

	void Envelope::Write(byte val) {
		m_reg = val;
	}

	void Envelope::Clock(AudioChannel* channel) {
		if (m_period) {
			m_counter++;

			if (m_counter == m_period) {
				m_counter = 0;

				if (m_direction) {
					if (m_volume < 0xF)
						m_volume++;
				}
				else {
					if (m_volume != 0)
						m_volume--;
				}
			}
		}
	}

	void Envelope::Reload(AudioChannel* channel) {
		m_init_volume = m_reg >> 4;
		m_direction = GET_BIT(m_reg, 3);
		m_period = m_reg & 0b111;

		m_volume = m_init_volume;

		m_counter = 0;
	}

	byte Envelope::GetVolume() const {
		return m_volume;
	}

	Sweep::Sweep() :
		m_reg(), m_direction(), m_period(),
		m_shifts(), m_clocks(), m_enabled(false),
		m_shadow()
	{}

	byte Sweep::Read() const {
		return m_reg;
	}

	void Sweep::Write(byte val) {
		m_reg = val;
	}

	void Sweep::Clock(AudioChannel* channel) {
		if (m_enabled) {
			m_clocks++;

			if (m_clocks >= m_period) {
				m_clocks = 0;

				if (m_shifts) {
					short shifted = m_shadow >> m_shifts;

					if (m_direction) {
						shifted = (short)m_shadow - shifted;

						if (shifted < 0) {
							return;
						}
					}
					else {
						shifted = m_shadow + shifted;
					}

					if (shifted > 2047) {
						channel->Disable();
					}
					else {
						m_shadow = (word)shifted;
						channel->SetFrequency((word)m_shadow);
					}
				}
			}
		}
	}

	void Sweep::Reload(AudioChannel* channel) {
		m_shadow = channel->GetFrequency();

		m_period = (m_reg >> 4) & 0b111;
		m_direction = GET_BIT(m_reg, 3);
		m_shifts = m_reg & 0b111;

		m_clocks = 0;

		if (m_period != 0) {
			m_enabled = true;
		}
	}
}