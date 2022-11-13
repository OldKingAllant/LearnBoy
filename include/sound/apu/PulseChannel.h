#pragma once

#include "Channel.h"
#include "EnvelopeSweep.h"
#include "LenCounter.h"
#include "Sequencer.h"

#include <array>

namespace GameboyEmu::Sound {
	constexpr std::array<byte, 8> wave_duty[] = {
		{1, 0, 0, 0, 0, 0, 0, 0}, 
		{1, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 0, 0, 0, 0, 1},
		{0, 1, 1, 1, 1, 1, 1, 0}
	};

	class PulseChannel : public AudioChannel {
	public :
		PulseChannel(bool use_sweep);

		void WriteFreq(byte value) override;
		void WriteControl(byte value) override;

		byte ReadFreq() const override;
		byte ReadControl() const override;

		byte ReadDuty() const;
		void WriteDuty(byte val);

		byte ReadEnvelope() const;
		void WriteEnvelope(byte value);

		byte ReadSweep() const;
		void WriteSweep(byte value);

		bool Clock() override;

		void Disable() override;
		void Restart() override;

		LenCounter* GetLenCounter() override;
		Sweep* GetSweep();
		Envelope* GetEnvelope();

		byte GetOutput() const override;

		~PulseChannel() override;

		word GetCalculatedPeriod() const override;

	private :
		byte m_duty_offset;
		byte m_duty_id;

		bool m_dac;

		Envelope m_envelope;
		LenCounter m_counter;
		Sweep* m_sweep;

		Sequencer<PulseChannel> m_seq;

		byte m_output;
		byte m_len;
	};
}