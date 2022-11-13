#include "../../include/datatransfer/SerialDevice.h"

namespace GameboyEmu::DataTransfer {
	SerialDevice::SerialDevice() :
		m_serial(nullptr)
	{}

	void SerialDevice::SetSerial(Serial* serial) {
		m_serial = serial;
	}

	SerialDevice::~SerialDevice() {}
}