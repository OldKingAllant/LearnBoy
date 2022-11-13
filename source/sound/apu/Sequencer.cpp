#include "../../../include/sound/apu/Sequencer.h"
#include "../../../include/sound/apu/PulseChannel.h"
#include "../../../include/sound/apu/NoiseChannel.h"
#include "../../../include/sound/apu/WaveChannel.h"

namespace GameboyEmu::Sound {
	Sequencer<PulseChannel>::Sequencer() :
	m_cycles(), m_step() {}

	void Sequencer<PulseChannel>::Clock(PulseChannel* channel) {
		if (m_cycles == 0) {
			switch (m_step)
			{
			case 0: 
			case 4: {
				if(channel->GetSelection())
					channel->GetLenCounter()->Clock(channel);
			} break;

			case 2:
			case 6: {
				if(channel->GetSweep())
					channel->GetSweep()->Clock(channel);
				if (channel->GetSelection())
					channel->GetLenCounter()->Clock(channel);
			} break;

			case 7: {
				channel->GetEnvelope()->Clock(channel);
			} break;

			default:
				break;
			}
		}

		m_cycles++;

		if (m_cycles == cycles_per_step) {
			m_cycles = 0;
			m_step = (m_step + 1) % 8;
		}
	}

	void Sequencer<PulseChannel>::Reset() {
		m_cycles = 0;
		m_step = 0;
	}




	Sequencer<NoiseChannel>::Sequencer() :
		m_cycles(), m_step() {}

	void Sequencer<NoiseChannel>::Clock(NoiseChannel* channel) {
		if (m_cycles == 0) {
			switch (m_step)
			{
			case 0:
			case 4: {
				if (channel->GetSelection())
					channel->GetLenCounter()->Clock(channel);
			} break;

			case 2:
			case 6: {
				if (channel->GetSelection())
					channel->GetLenCounter()->Clock(channel);
			} break;

			case 7: {
				channel->GetEnvelope()->Clock(channel);
			} break;

			default:
				break;
			}
		}

		m_cycles++;

		if (m_cycles == cycles_per_step) {
			m_cycles = 0;
			m_step = (m_step + 1) % 8;
		}
	}

	void Sequencer<NoiseChannel>::Reset() {
		m_cycles = 0;
		m_step = 0;
	}



	Sequencer<WaveChannel>::Sequencer() :
		m_cycles(), m_step() {}

	void Sequencer<WaveChannel>::Clock(WaveChannel* channel) {
		if (m_cycles == 0) {
			switch (m_step)
			{
			case 0:
			case 4: {
				if (channel->GetSelection())
					channel->GetLenCounter()->Clock(channel);
			} break;

			case 2:
			case 6: {
				if (channel->GetSelection())
					channel->GetLenCounter()->Clock(channel);
			} break;

			case 7: {
				//
			} break;

			default:
				break;
			}
		}

		m_cycles++;

		if (m_cycles == cycles_per_step) {
			m_cycles = 0;
			m_step = (m_step + 1) % 8;
		}
	}

	void Sequencer<WaveChannel>::Reset() {
		m_cycles = 0;
		m_step = 0;
	}
}