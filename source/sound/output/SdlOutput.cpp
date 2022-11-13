#include "../../../include/sound/output/SdlOutput.h"

#include <iostream>

namespace GameboyEmu::Sound {
	SdlOutputDevice::SdlOutputDevice(Logger& logger) :
	m_freq(), m_silence(), 
	m_size(), m_curr_size(), 
	m_device_id{},
	m_logger(logger), m_buffer(nullptr) {
		m_buffer = new byte[8192];
	}

	void audio_callback(void* userdata, byte* stream, int len) {
		SdlOutputDevice* dev = reinterpret_cast<SdlOutputDevice*>(userdata);
		byte* data = stream;

		if (dev->m_curr_size == 0) {
			std::fill_n(data, len, 0);
		}
		else {
			SDL_memcpy(stream, dev->m_buffer, len);

			//std::copy_n(dev->m_buffer, len, data);
			//std::fill_n(data, len, 0xFFFF);
			dev->m_curr_size = 0;
		}
	}

	void SdlOutputDevice::Init() {
		if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
			LOG_ERR(m_logger, "Fatal error, could not init audio\n");
			std::exit(0);
		}
		

		SDL_AudioSpec want, have;

		SDL_zero(want);

		want.freq = 44100;
		want.channels = 2;
		want.samples = 2048;
		want.format = AUDIO_U8;
		want.callback = audio_callback;
		want.userdata = this;

		m_device_id = SDL_OpenAudioDevice(NULL,
			0, &want, &have, 0);

		m_silence = have.silence;

		SDL_PauseAudioDevice(m_device_id, 0);
	}

	std::string const& SdlOutputDevice::GetDeviceName() const {
		return "SdlOutputDevice";
	}

	int SdlOutputDevice::GetFrequency() const {
		return 0;
	}

	byte SdlOutputDevice::GetSilence() const {
		return 0;
	}

	unsigned SdlOutputDevice::GetBufferSize() const {
		return 0;
	}

	void SdlOutputDevice::SendSamples(byte* buffer) {
		SDL_LockAudio();
		//auto size = SDL_GetQueuedAudioSize(m_device_id);

		/*if (size > 0) {
			SDL_Delay(5);
		}*/

		//SDL_QueueAudio(m_device_id, buffer, 4096);

		std::copy(buffer, buffer + 4096, m_buffer);
		m_curr_size = 4096;

		SDL_UnlockAudio();
	}

	SdlOutputDevice::~SdlOutputDevice() {
		if (SDL_WasInit(SDL_INIT_AUDIO)) {
			SDL_QuitSubSystem(SDL_INIT_AUDIO);
		}

		delete m_buffer;
	}
}