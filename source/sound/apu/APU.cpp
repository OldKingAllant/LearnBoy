#include "../../../include/sound/apu/APU.h"
#include "../../../include/logging/Logger.h"
#include "../../../include/state/EmulatorState.h"
#include "../../../include/sound/apu/Channel.h"
#include "../../../include/sound/apu/PulseChannel.h"
#include "../../../include/sound/apu/NoiseChannel.h"
#include "../../../include/sound/apu/WaveChannel.h"

#define CAST(tp, ch)  dynamic_cast<tp*>(ch)

namespace GameboyEmu::Sound {
	APU::APU(State::EmulatorState* state, OutputDevice* outdev) 
	: m_state(state), m_dev(outdev), 
	m_ch1(nullptr), m_ch2(nullptr), m_ch3(nullptr), 
	m_ch4(nullptr), m_left_vol(), m_right_vol(),
	m_enabled(false), m_panning{}, m_samples(nullptr), 
		m_num_samples(), m_curr_cycles(), m_capacitor(0)
	{
		m_ch1 = new PulseChannel(true);
		m_ch2 = new PulseChannel(false);
		m_ch3 = new WaveChannel();
		m_ch4 = new NoiseChannel();

		m_samples = new byte[num_samples];
	}

	void APU::WriteReg(word address, byte value) {
		if (address >= 0xFF30 && address < 0xFF40) {
			CAST(WaveChannel, m_ch3)->WriteWaveRam(
				address - 0xFF30, value
			);

			return;
		}
		
		switch (address)
		{
		case 0xFF10: {
			CAST(PulseChannel, m_ch1)->WriteSweep(value);
		} break;

		case 0xFF11: {
			CAST(PulseChannel, m_ch1)->WriteDuty(value);
		} break;

		case 0xFF12: {
			CAST(PulseChannel, m_ch1)->WriteEnvelope(value);
		} break;

		case 0xFF13: {
			CAST(PulseChannel, m_ch1)->WriteFreq(value);
		} break;

		case 0xFF14: {
			CAST(PulseChannel, m_ch1)->WriteControl(value);
		} break;

		case 0xFF16: {
			CAST(PulseChannel, m_ch2)->WriteDuty(value);
		} break;

		case 0xFF17: {
			CAST(PulseChannel, m_ch2)->WriteEnvelope(value);
		} break;

		case 0xFF18: {
			CAST(PulseChannel, m_ch2)->WriteFreq(value);
		} break;

		case 0xFF19: {
			CAST(PulseChannel, m_ch2)->WriteControl(value);
		} break;

		case 0xFF1A: {
			CAST(WaveChannel, m_ch3)->SetEnabled(value);
		} break;

		case 0xFF1B: {
			CAST(WaveChannel, m_ch3)->WriteLen(value);
		} break;

		case 0xFF1C: {
			CAST(WaveChannel, m_ch3)->WriteOutLevel(value);
		} break;

		case 0xFF1D: {
			CAST(WaveChannel, m_ch3)->WriteFreq(value);
		} break;

		case 0xFF1E: {
			CAST(WaveChannel, m_ch3)->WriteControl(value);
		} break;

		case 0xFF20: {
			CAST(NoiseChannel, m_ch4)->WriteLen(value);
		} break;

		case 0xFF21: {
			CAST(NoiseChannel, m_ch4)->WriteEnvelope(value);
		} break;

		case 0xFF22: {
			CAST(NoiseChannel, m_ch4)->WritePolynomialCounter(value);
		} break;

		case 0xFF23: {
			CAST(NoiseChannel, m_ch4)->WriteControl(value);
		} break;

		case 0xFF24: {
			m_left_vol = (value >> 4) & 0b111;
			m_right_vol = value & 0b111;
		} break;

		case 0xFF25: {
			byte ch4_l = GET_BIT(value, 7);
			byte ch3_l = GET_BIT(value, 6);
			byte ch2_l = GET_BIT(value, 5);
			byte ch1_l = GET_BIT(value, 4);

			byte ch4_r = GET_BIT(value, 3);
			byte ch3_r = GET_BIT(value, 2);
			byte ch2_r = GET_BIT(value, 1);
			byte ch1_r = value & 1;

			m_panning[0] = (Panning) ((ch1_r << 1) | ch1_l);
			m_panning[1] = (Panning) ((ch2_r << 1) | ch2_l);
			m_panning[2] = (Panning) ((ch3_r << 1) | ch3_l);
			m_panning[3] = (Panning) ((ch4_r << 1) | ch4_l);
		} break;

		case 0xFF26: {
			m_enabled = GET_BIT(value, 7);
		} break;

		default:
			//LOG_WARN(m_state->GetLogger(), "Invalid audio register\n");
			break;
		}
	}

	byte APU::ReadReg(word address) {
		if (address >= 0xFF30 && address < 0xFF40) {
			return CAST(WaveChannel, m_ch3)->ReadWaveRam(
				address - 0xFF30
			);
		}

		switch (address)
		{
		case 0xFF10: {
			return CAST(PulseChannel, m_ch1)->ReadSweep();
		} break;

		case 0xFF11: {
			return CAST(PulseChannel, m_ch1)->ReadDuty();
		} break;

		case 0xFF12: {
			return CAST(PulseChannel, m_ch1)->ReadEnvelope();
		} break;

		case 0xFF13: {
			return CAST(PulseChannel, m_ch1)->ReadFreq();
		} break;

		case 0xFF14: {
			return CAST(PulseChannel, m_ch1)->ReadControl();
		} break;

		case 0xFF16: {
			return CAST(PulseChannel, m_ch2)->ReadDuty();
		} break;

		case 0xFF17: {
			return CAST(PulseChannel, m_ch2)->ReadEnvelope();
		} break;

		case 0xFF18: {
			return CAST(PulseChannel, m_ch2)->ReadFreq();
		} break;

		case 0xFF19: {
			return CAST(PulseChannel, m_ch2)->ReadControl();
		} break;

		case 0xFF1A: {
			byte en = CAST(WaveChannel, m_ch3)->Enabled();
			
			return (en << 7);
		} break;

		case 0xFF1B: {
			return 0xFF;
		} break;

		case 0xFF1C: {
			return CAST(WaveChannel, m_ch3)->ReadOutLevel();
		} break;

		case 0xFF1D: {
			return 0xFF;
		} break;

		case 0xFF1E: {
			return CAST(WaveChannel, m_ch3)->ReadControl();
		} break;

		case 0xFF20: {
			return CAST(NoiseChannel, m_ch4)->ReadLen();
		} break;

		case 0xFF21: {
			return CAST(NoiseChannel, m_ch4)->ReadEnvelope();
		} break;

		case 0xFF22: {
			return CAST(NoiseChannel, m_ch4)->ReadPolynomialCounter();
		} break;

		case 0xFF23: {
			return CAST(NoiseChannel, m_ch4)->ReadControl();
		} break;

		case 0xFF24: {
			return (m_left_vol << 4) |
				(m_right_vol);
		} break;

		case 0xFF25: {
			return 0xFF;
		} break;

		case 0xFF26: {
			return (!!(m_enabled) << 7) |
				(!!(m_ch4->Enabled()) << 3) |
				(!!(m_ch3->Enabled()) << 2) |
				(!!(m_ch2->Enabled()) << 1) |
				(!!(m_ch1->Enabled()));
		} break;

		default:
			//LOG_WARN(m_state->GetLogger(), "Invalid audio register\n");
			break;
		}

		return 0xFF;
	}

	void APU::Tick(byte cycles) {
		word tstates = cycles * 4;

		while (tstates) {
			if (m_enabled) {
				m_curr_cycles++;

				m_ch1->Clock();
				m_ch2->Clock();
				m_ch3->Clock();
				m_ch4->Clock();

				if (m_curr_cycles >= sample_clocks) {
					std::array<short, 4> samples{
						m_ch1->GetOutput(),
						m_ch2->GetOutput(),
						m_ch3->GetOutput(),
						m_ch4->GetOutput()
					};

					m_curr_cycles = 0;

					mix_samples(samples);
				}
			}

			tstates--;
		}
	}

	float APU::high_pass(float in) {
		float out = 0.0f;

		/*if (m_enabled) {
			out = in - m_capacitor;

			m_capacitor = in - out * high_pass_rate;
		}*/

		return out;
	}

	void APU::mix_samples(std::array<short, 4> const& samples) {
		auto filter = [this](short val, byte ch_id) {
			auto res = std::pair<byte, byte>{};

			if (m_panning[ch_id] == Panning::mute) {
				res.first = 0;
				res.second = 0;
			}
			else {
				if (m_panning[ch_id] == Panning::left ||
					m_panning[ch_id] == Panning::middle) {
					res.first = (byte)val
						* (m_left_vol + 1);
				}
				else {
					res.first = 0;
				}

				if (m_panning[ch_id] == Panning::right ||
					m_panning[ch_id] == Panning::middle) {
					res.second = (byte)val *
						(m_right_vol + 1);
				}
				else {
					res.second = 0;
				}
			}

			return res;
		};

		word mixed_left = 0;
		word mixed_right = 0;

		auto res = filter(samples[0], 0);

		mixed_left = res.first;
		mixed_right = res.second;

		res = filter(samples[1], 1);

		mixed_left += res.first;
		mixed_right += res.second;

		res = filter(samples[2], 2);

		mixed_left += res.first;
		mixed_right += res.second;

		res = filter(samples[3], 3);

		mixed_left += res.first;
		mixed_right += res.second;

		mixed_left /= 4;
		mixed_right /= 4;

		m_samples[m_num_samples++] = (byte)mixed_left;
		m_samples[m_num_samples++] = (byte)mixed_right;

		if (m_num_samples == num_samples) {
			if (m_dev) {
				m_dev->SendSamples(m_samples);
			}

			m_num_samples = 0;
		}
	}

	std::size_t APU::DumpState(byte* buffer, std::size_t offset) {
		buffer[offset] = m_left_vol;
		buffer[offset + 1] = m_right_vol;
		buffer[offset + 2] = m_enabled;

		buffer[offset + 3] = (byte)m_panning[0];
		buffer[offset + 4] = (byte)m_panning[1];
		buffer[offset + 5] = (byte)m_panning[2];
		buffer[offset + 6] = (byte)m_panning[3];

		offset += 7;

		std::copy_n(m_samples, num_samples, buffer + offset);

		offset += num_samples;

		WriteWord(buffer, offset, m_num_samples);
		WriteWord(buffer, offset + 2, m_curr_cycles);

		return offset + 4;
	}

	std::size_t APU::LoadState(byte* buffer, std::size_t offset) {
		m_left_vol = buffer[offset];
		m_right_vol = buffer[offset + 1];
		m_enabled = buffer[offset + 2];

		m_panning[0] = (Panning)buffer[offset + 3];
		m_panning[1] = (Panning)buffer[offset + 4];
		m_panning[2] = (Panning)buffer[offset + 5];
		m_panning[3] = (Panning)buffer[offset + 6];

		offset += 7;

		std::copy_n(buffer + offset, num_samples, m_samples);

		offset += num_samples;

		m_num_samples = ReadWord(buffer, offset);
		m_curr_cycles = ReadWord(buffer, offset + 2);

		return offset + 4;
	}

	APU::~APU() {
		delete m_ch1;
		delete m_ch2;
		delete m_ch4;
		delete m_samples;
	}
}