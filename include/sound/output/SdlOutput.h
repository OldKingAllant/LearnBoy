#pragma once

#include "OutputDevice.h"
#include "SDL2/SDL.h"
#include "../../logging/Logger.h"

namespace GameboyEmu::Sound {
	class SdlOutputDevice : public OutputDevice {
	public :
		SdlOutputDevice(Logger& logger);

		std::string const& GetDeviceName() const override;

		int GetFrequency() const override;
		byte GetSilence() const override;
		unsigned GetBufferSize() const override;

		void SendSamples(byte* buffer) override;

		void Init() override;

		~SdlOutputDevice();

		friend void audio_callback(void* userdata, byte* stream, int len);

	private :
		unsigned m_freq;
		byte m_silence;
		unsigned m_size;

		unsigned m_curr_size;

		SDL_AudioDeviceID m_device_id;

		Logger& m_logger;

		byte* m_buffer;
	};
}