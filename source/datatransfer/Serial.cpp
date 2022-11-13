#include "../../include/datatransfer/Serial.h"
#include "../../include/datatransfer/SerialDevice.h"
#include "../../include/memory/Memory.h"


#include <iostream>

namespace GameboyEmu::DataTransfer {
#define LOCK() 

	Serial::Serial(SerialDevice* dev) :
		m_data_transfer(), m_flag(),
		m_clock(), m_curr_cycles(0),
		m_mmu(nullptr), m_dev(dev) {
		m_dev->SetSerial(this);
	}

	void Serial::WriteData(byte val) {
		LOCK();

		m_data_transfer = val;

		m_dev->SetOut(val);
	}

	byte Serial::ReadData() {
		LOCK();

		return m_data_transfer;
	}

	void Serial::WriteControl(byte val) {
		byte f = GET_BIT(val, 7);
		byte clk = val & 1;

		LOCK();

		m_flag = f;
		m_clock = (ClockType)clk;

		m_curr_cycles = 0;

		m_dev->SetClockType(clk);
	}

	byte Serial::ReadControl() {
		LOCK();

		return (m_flag << 7) | (byte)m_clock;
	}

	void Serial::Clock(byte cycles) {
		word tstates = cycles * 4;

		while (tstates && m_flag) {
			m_curr_cycles++;

			if (m_curr_cycles >= 8192) {
				m_curr_cycles -= 8192;

				if (m_clock == ClockType::internal) {
					byte in = 0xFF;

					bool r = m_dev->Send(in);

					//if(r)
					RequestInterrupt(in);
				}
				
			}

			tstates--;
		}
	}

	void Serial::SetMemory(Mem::Memory* mmu) {
		m_mmu = mmu;
	}

	void Serial::Listen(std::string const& on) {
		m_dev->Listen(on);
	}

	void Serial::Connect(std::string const& to) {
		m_dev->Connect(to);
	}

	void Serial::CloseConnection() {
		m_dev->CloseConnection();
	}

	void Serial::RequestInterrupt(byte data) {
		m_flag = 0;
		m_data_transfer = data;

		byte ir = m_mmu->Read(0xFF0F);

		SERIAL_BIT_SET(ir);

		m_mmu->Write(0xFF0F, ir);
	}

	Serial::~Serial() {
		if (m_dev) {
			CloseConnection();
			delete m_dev;
		}
	}

	std::size_t Serial::DumpState(byte* buffer, std::size_t offset) {
		buffer[offset] = m_data_transfer.load();
		buffer[offset + 1] = m_flag.load();
		buffer[offset + 2] = (byte)m_clock;

		WriteWord(buffer, offset + 3, m_curr_cycles);

		return offset + 5;
	}

	std::size_t Serial::LoadState(byte* buffer, std::size_t offset) {
		m_data_transfer = buffer[offset];
		m_flag = buffer[offset + 1];
		m_clock = (ClockType)buffer[offset + 2];

		m_curr_cycles = ReadWord(buffer, offset + 3);

		return offset + 5;
	}
}