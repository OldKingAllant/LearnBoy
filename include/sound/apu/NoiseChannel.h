#pragma once

#include "Channel.h"
#include "EnvelopeSweep.h"
#include "LenCounter.h"
#include "Sequencer.h"

namespace GameboyEmu::Sound {
	static constexpr byte divs[] = {
		 8,
         16,
         32,
         48,
         64,
         80,
         96,
         112
	};

	class NoiseChannel : public AudioChannel {
	public :
		NoiseChannel();

		void WriteFreq(byte value) override;
		void WriteControl(byte value) override;

		byte ReadFreq() const override;
		byte ReadControl() const override;

		void WritePolynomialCounter(byte value);
		byte ReadPolynomialCounter() const;

		byte ReadEnvelope() const;
		void WriteEnvelope(byte value);

		bool Clock() override;

		void Disable() override;
		void Restart() override;

		LenCounter* GetLenCounter() override;
		Envelope* GetEnvelope();

		~NoiseChannel() override;

		byte GetOutput() const override;

		void WriteLen(byte value);
		byte ReadLen() const;

		word GetCalculatedPeriod() const override;

	private :
		void clock_lfsr();

	private :
		Envelope m_envelope;
		LenCounter m_counter;
		Sequencer<NoiseChannel> m_seq;
		//SoundTimer m_timer;

		byte m_len;

		byte m_shift_freq;
		byte m_step;
		byte m_ratio;

		byte m_output;

		word m_lfsr;
	};
}