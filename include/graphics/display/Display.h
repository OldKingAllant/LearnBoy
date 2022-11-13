#pragma once

#include <SDL2/SDL.h>

#include "../../common/Common.h"

#include <thread>
#include <atomic>

#include "../../logging/Logger.h"
#include "../../input/Joypad.h"

#include <functional>

namespace GameboyEmu {
	namespace Graphics {

		class Display {
		private:
			unsigned m_width;
			unsigned m_height;
			unsigned m_scaling;

			SDL_Window* m_window;
			SDL_Renderer* m_renderer;
			SDL_Surface* m_surface;
			SDL_Texture* m_texture;

			std::thread m_render_thread;

			std::atomic<bool> m_frame_present;
			std::atomic<bool> m_stop;

			std::mutex m_buffer_mutex;

			byte* m_pixel_buffer;

			Logger& m_log;

			Input::Joypad* m_joypad;

			std::function<void()> m_ctrl_c_fun;

			bool m_ctrl_status;

			static constexpr unsigned buffer_size = 160 * 144 * 4;

		private:
			void ScaleAndSetPixel(unsigned i, unsigned j, unsigned* dest);
			void SetPixels(unsigned* dest);

			void KeyboardDown(SDL_KeyboardEvent* ev);
			void KeyboardUp(SDL_KeyboardEvent* ev);

		public:
			Display(Logger& logger, std::function<void()> ctrl_c);

			bool Init(unsigned w, unsigned h, unsigned scale);

			void Loop();

			void Render();

			void SetFrame(byte* buffer);

			void ProcessEvent(SDL_Event* ev);

			bool IsStop();

			void Stop();

			void FramePresent();

			void SetJoypad(Input::Joypad* joypad);

			~Display();
		};
	}
}