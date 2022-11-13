#pragma once

#include "../common/Common.h"

namespace GameboyEmu::DataTransfer {
	class Serial;

	class SerialDevice {
	public :
		SerialDevice();

		virtual bool Send(byte& out) = 0;
		virtual void SetOut(byte out) = 0;

		virtual void Listen(std::string const& on) = 0;
		virtual void Connect(std::string const& to) = 0;
		virtual void CloseConnection() = 0;

		virtual void SetClockType(byte type) = 0;

		void SetSerial(Serial* serial);

		virtual ~SerialDevice();

		virtual bool Connected() const = 0;

	protected :
		Serial* m_serial;
	};
}