#pragma once

#include "../../common/Common.h"
#include <string>

namespace GameboyEmu::Sound {
	class OutputDevice {
	public :
		OutputDevice() = default;

		virtual std::string GetDeviceName() const = 0;

		virtual int GetFrequency() const = 0;
		virtual byte GetSilence() const = 0;
		virtual unsigned GetBufferSize() const = 0;

		virtual void SendSamples(byte* buffer) = 0;

		virtual void Init() = 0;

		virtual ~OutputDevice() {}
	};
}