#pragma once

#include "../common/Common.h"

#include <mutex>

namespace GameboyEmu::Mem {
	class Memory;
}

namespace GameboyEmu::Input {
	class Joypad {
	public :
		Joypad();

		
		/// ////
		
		void SetStart();
		void UnsetStart();

		void SetSelect();
		void UnsetSelect();

		void SetA();
		void UnsetA();

		void SetB();
		void UnsetB();

		/// ////

		void SetDown();
		void UnsetDown();

		void SetUp();
		void UnsetUp();

		void SetLeft();
		void UnsetLeft();

		void SetRight();
		void UnsetRight();

		/// ////

		void Write(byte val);
		byte Read();

		void RequestInterrupt();

		void SetMemory(Mem::Memory* mem);

	private :
		std::mutex m_mutex;

		struct joypad_ctx {
			byte m_select_actions;
			byte m_select_direction;

			byte m_status[2][4];
		};
		
		joypad_ctx m_ctx;

		Mem::Memory* m_mem;
	};
}