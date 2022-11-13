#include "../../include/timing/Timer.h"
#include "../../include/memory/Memory.h"

namespace GameboyEmu {
	namespace Timing {

		Timer::Timer() : m_divider(0),
			m_tima(0), m_tma(0), m_enable(0), m_clock_select(0),
			m_real_clock_cycle(0), m_current_cycles(0),
			m_divider_cycles(0), m_mem(nullptr) {}

		void Timer::SetMemory(Mem::Memory* mmu) {
			m_mem = mmu;
		}

		void Timer::SetDiv(byte) {
			m_divider = 0;
			m_divider_cycles = 0;
		}

		void Timer::SetTima(byte value) {
			m_tima = value;
			//current_cycles = 0;
		}

		void Timer::SetTma(byte value) {
			m_tma = value;
		}

		void Timer::SetTAC(byte options) {
			//bit 2 is timer enable
			m_enable = (options >> 2) & 1;

			//bit 0-1 is clock cycle
			m_clock_select = (options & 0b11);

			m_current_cycles = 0;

			switch (m_clock_select)
			{
			case 0x00:
				m_real_clock_cycle = 1024;
				break;
			case 0x01:
				m_real_clock_cycle = 16;
				break;
			case 0x02:
				m_real_clock_cycle = 64;
				break;
			case 0x03:
				m_real_clock_cycle = 256;
				break;
			}
		}

		byte Timer::GetDiv() const {
			return m_divider;
		}

		byte Timer::GetTima() const {
			return m_tima;
		}

		byte Timer::GetTma() const {
			return m_tma;
		}

		byte Timer::GetTAC() const {
			return (m_enable << 2) | m_clock_select;
		}

		void Timer::Tick(byte mcycles) {
			unsigned tstates = mcycles * 4;

			m_divider_cycles += tstates;

			if (m_divider_cycles >= 256) {
				m_divider++;
				m_divider_cycles -= 256;
			}

			if (!m_enable)
				return;

			m_current_cycles += tstates;

			if (m_current_cycles >=
				m_real_clock_cycle) {
				m_tima++;

				m_current_cycles -= m_real_clock_cycle;

				if (m_tima == 0) {
					m_tima = m_tma;

					byte ir = m_mem->Read(0xFF0F);

					TIMER_BIT_SET(ir);

					m_mem->Write(0xFF0F, ir);
				}
			}
		}

		std::size_t Timer::DumpState(byte* buffer, std::size_t offset) {
			buffer[offset] = m_divider;
			buffer[offset + 1] = m_tima;
			buffer[offset + 2] = m_tma;
			buffer[offset + 3] = m_enable;
			buffer[offset + 4] = m_clock_select;

			WriteWord(buffer, offset + 5, (word)(m_real_clock_cycle & 0xFFFF));
			WriteWord(buffer, offset + 7, (word)((m_real_clock_cycle >> 16) & 0xFFFF));
			WriteWord(buffer, offset + 9, (word)(m_current_cycles & 0xFFFF));
			WriteWord(buffer, offset + 11, (word)((m_current_cycles >> 16) & 0xFFFF));
			WriteWord(buffer, offset + 13, (word)(m_divider_cycles & 0xFFFF));
			WriteWord(buffer, offset + 15, (word)((m_divider_cycles >> 16) & 0xFFFF));

			return offset + 17;
		}

		std::size_t Timer::LoadState(byte* buffer, std::size_t offset) {
			m_divider = buffer[offset];
			m_tima = buffer[offset + 1];
			m_tma = buffer[offset + 2];
			m_enable = buffer[offset + 3];
			m_clock_select = buffer[offset + 4];

			m_real_clock_cycle =  ReadWord(buffer, offset + 5);
			m_real_clock_cycle |= (ReadWord(buffer, offset + 7) << 16);

			m_current_cycles = ReadWord(buffer, offset + 9);
			m_current_cycles |= (ReadWord(buffer, offset + 11) << 16);

			m_divider_cycles = ReadWord(buffer, offset + 13);
			m_divider_cycles |= (ReadWord(buffer, offset + 15) << 16);

			return offset + 17;
		}
	}
}