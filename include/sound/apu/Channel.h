#pragma once

#include "../../common/Common.h"

#include "SoundTimer.h"

namespace GameboyEmu::Sound {
	class LenCounter;

	class AudioChannel {
	public :
		AudioChannel();

		virtual void WriteFreq(byte value) = 0;
		virtual void WriteControl(byte value) = 0;

		virtual byte ReadFreq() const = 0;
		virtual byte ReadControl() const = 0;

		virtual bool Clock() = 0;

		virtual void Disable() = 0;
		virtual void Restart() = 0;

		virtual LenCounter* GetLenCounter() = 0;

		bool Enabled() const;

		word GetFrequency() const;
		void SetFrequency(word freq);

		virtual byte GetOutput() const = 0;

		byte GetSelection() const;

		virtual ~AudioChannel();

		virtual word GetCalculatedPeriod() const = 0;

	protected :
		word m_freq;

		byte m_counter_select;

		bool m_enabled;

		SoundTimer m_timer;

		byte m_selection;
	};
}