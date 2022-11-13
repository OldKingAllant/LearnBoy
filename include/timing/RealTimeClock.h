#pragma once

#include "../common/Common.h"

namespace GameboyEmu::Timing {
	class RealTimeClock {
	public :
		RealTimeClock();

		byte ReadRegister(byte id) const;
		void WriteRegister(byte id, byte value);

		void LatchClock(byte val);

	private :
		std::tm GetTimePoint() const;

	private :
		byte m_last_written;

		byte m_seconds;
		byte m_minutes;
		byte m_hours;

		word m_days;

		byte m_halt;
		byte m_carry;
	};
}