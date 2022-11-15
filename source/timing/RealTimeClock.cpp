#include "../../include/timing/RealTimeClock.h"

#include <chrono>
#include <ctime>

namespace GameboyEmu::Timing {
	RealTimeClock::RealTimeClock() :
		m_last_written(0xFF), m_seconds(0),
		m_minutes(), m_hours(), m_days(),
		m_halt(), m_carry()
	{}

	std::tm RealTimeClock::GetTimePoint() const {
		auto now = std::chrono::system_clock::now();

		std::time_t time = std::chrono::system_clock::to_time_t(now);

		std::tm time_point{};

		localtime_s(&time_point, &time);

		return time_point;
	}

	byte RealTimeClock::ReadRegister(byte id) const {
		switch (id)
		{
		case 0:
			return m_seconds;
			break;

		case 1: 
			return m_minutes;
			break;

		case 2:
			return m_hours;
			break;

		case 3:
			return (byte)(m_days & 0xFF);
			break;

		case 4: {
			return (m_carry << 7) |
				(m_halt << 6) |
				((m_days >> 8) & 1);
		} break;

		default:
			break;
		}

		return 0xFF;
	}
	void RealTimeClock::WriteRegister(byte id, byte value) {
		if (!m_halt)
			return;

		switch (id)
		{
		case 0:
			m_seconds = value;
			break;

		case 1:
			m_minutes = value;
			break;

		case 2:
			m_hours = value;
			break;

		case 3:
			m_days = (m_days & 0x100) | value;
			break;

		case 4: {
			m_days = ((value & 1) << 8) | m_days;
			m_halt = GET_BIT(value, 6);
			m_carry = GET_BIT(value, 7);
		} break;

		default:
			break;
		}
	}

	void RealTimeClock::LatchClock(byte val) {
		if (m_halt)
			return;

		if (m_last_written == 0 && val != 0) {
			//Latch
			std::tm time_p = GetTimePoint();

			m_seconds = time_p.tm_sec;
			m_minutes = time_p.tm_min;
			m_hours = time_p.tm_hour;

			m_days = time_p.tm_yday;
		}

		m_last_written = val;
	}
}