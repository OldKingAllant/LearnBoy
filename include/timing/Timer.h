#pragma once

#include "../common/Common.h"

namespace GameboyEmu {
	namespace Mem {
		class Memory;
	}

	namespace Timing {

		class Timer {
		private:
			//Incremented at a rate of 16384Hz
			//always incremented (when the cpu is not STOPPED)
			byte m_divider;

			//Incremented at the frequence specified by
			//the TAC register
			//On overflow, it is reset at the value
			//set in TMA and an interrupt is requested
			byte m_tima;

			//Timer modulo, the value that is assigned
			//to TIMA on overflow
			byte m_tma;

			//TAC
			byte m_enable;

			byte m_clock_select;

			unsigned m_real_clock_cycle;

			unsigned m_current_cycles;
			unsigned m_divider_cycles;

			Mem::Memory* m_mem;

		public:
			Timer();

			void SetMemory(Mem::Memory* mmu);

			//Writes to Div will reset the register 
			//to 0 (the argument is effectively ignored)
			void SetDiv(byte);

			void SetTima(byte value);

			void SetTma(byte value);

			void SetTAC(byte options);

			byte GetDiv() const;
			byte GetTima() const;
			byte GetTma() const;
			byte GetTAC() const;

			void Tick(byte cycles);

			std::size_t DumpState(byte* buffer, std::size_t offset);
			std::size_t LoadState(byte* buffer, std::size_t offset);
		};
	}
}