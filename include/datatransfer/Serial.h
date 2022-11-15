#pragma once

#include "../common/Common.h"

#include <mutex>
#include <atomic>

namespace GameboyEmu::Mem {
	class Memory;
}

namespace GameboyEmu::DataTransfer {
	class SerialDevice;

	enum class ClockType {
		external,
		internal
	};

	class Serial {
	public :
		Serial(SerialDevice* dev);

		void WriteData(byte val);
		byte ReadData();

		void WriteControl(byte val);
		byte ReadControl();

		void Clock(byte cycles);

		void SetMemory(Mem::Memory* mmu);

		void Listen(std::string const& on);
		void Connect(std::string const& to);
		void CloseConnection();

		~Serial();

		void RequestInterrupt(byte data);

		std::size_t DumpState(byte* buffer, std::size_t offset);
		std::size_t LoadState(byte* buffer, std::size_t offset);

		static constexpr unsigned clock_rate = 512;

	private :
		std::atomic_uchar m_data_transfer;
		std::atomic_uchar m_flag;
		ClockType m_clock;

		word m_curr_cycles;

		Mem::Memory* m_mmu;
		SerialDevice* m_dev;
	};
}