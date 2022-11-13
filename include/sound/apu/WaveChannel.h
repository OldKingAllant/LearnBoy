#pragma once

#include "Channel.h"
#include "EnvelopeSweep.h"
#include "LenCounter.h"
#include "SoundTimer.h"
#include "Sequencer.h"

namespace GameboyEmu::Sound {
	class WaveChannel : public AudioChannel {
	public :
		WaveChannel();

		void WriteFreq(byte value) override;
		void WriteControl(byte value) override;

		byte ReadFreq() const override;
		byte ReadControl() const override;

		bool Clock() override;

		void Disable() override;
		void Restart() override;

		LenCounter* GetLenCounter() override;

		byte GetOutput() const override;

		~WaveChannel();

		word GetCalculatedPeriod() const override;

		void WriteLen(byte value);
		
		void WriteOutLevel(byte value);
		byte ReadOutLevel() const;

		void WriteWaveRam(byte offset, byte value);
		byte ReadWaveRam(byte offset);

		void SetEnabled(byte en);

	private :
		byte m_len;
		byte m_volume;
		word m_wavelen;
		byte m_output;
		bool m_dac;

		byte m_curr_sample;

		LenCounter m_counter;
		//SoundTimer m_timer;
		Sequencer<WaveChannel> m_seq;

		byte m_waveram[16];
	};
}