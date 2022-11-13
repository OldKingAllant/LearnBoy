#include "../../../include/graphics/display/Display.h"

namespace GameboyEmu::Graphics {

	Display::Display(Logger& logger, std::function<void()> ctrl_c) :
		m_width(), m_height(), m_scaling(),
		m_window(nullptr),
		m_renderer(nullptr), m_surface(nullptr),
		m_texture(nullptr),
		m_render_thread(), m_frame_present(),
		m_stop(), m_pixel_buffer(nullptr),
		m_buffer_mutex(),
		m_log(logger), m_joypad(nullptr), 
		m_ctrl_c_fun(ctrl_c), m_ctrl_status(false) {
		m_pixel_buffer = new byte[buffer_size];

		std::fill_n(m_pixel_buffer,
			buffer_size, 0xFF);
	}

	bool Display::Init(unsigned w, unsigned h, unsigned scale) {
		m_width = w;
		m_height = h;
		m_scaling = scale;

		m_render_thread = std::thread([this]() {
			//log.log_info("LCD Init\n");

			if (SDL_Init(SDL_INIT_VIDEO) < 0) {
				m_log.log_err("SDL Init failed : {}\n", SDL_GetError());;
			}

			m_window = SDL_CreateWindow(
				"GB Emulator", SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED, m_width * m_scaling,
				m_height * m_scaling, SDL_WINDOW_SHOWN
			);

			if (m_window == NULL) {
				m_log.log_err("SDL Create window failed : {}\n", SDL_GetError());
			}

			m_renderer = SDL_CreateRenderer(m_window, -1,
				SDL_RENDERER_ACCELERATED);

			if (m_renderer == NULL) {
				m_log.log_err("SDL Create renderer failed : {}\n", SDL_GetError());
			}

			m_texture = SDL_CreateTexture(m_renderer,
				SDL_PIXELFORMAT_ARGB8888,
				SDL_TEXTUREACCESS_STREAMING,
				m_width * m_scaling, m_height * m_scaling);

			//log.log_info("LCD Init successfull\n");

			this->Loop();
			});

		return true;
	}

	void Display::FramePresent() {
		m_frame_present.store(true);
	}

	void Display::ScaleAndSetPixel(
		unsigned i, unsigned j, unsigned* dest) {
		unsigned y_start = i;
		unsigned x_start = j;

		unsigned y_end = y_start + m_scaling;
		unsigned x_end = x_start + m_scaling;

		unsigned index = (i * m_width + j) * 4;

		byte b = m_pixel_buffer[index];
		byte g = m_pixel_buffer[index + 1];
		byte r = m_pixel_buffer[index + 2];
		byte a = m_pixel_buffer[index + 3];

		for (unsigned y = y_start;
			y < y_end && y < m_height; y++) {
			for (unsigned y_scale = 0; y_scale < m_scaling;
				y_scale++) {
				for (unsigned x = x_start;
					x < x_end && x < m_width; x++) {
					for (unsigned x_scale_pos = 0;
						x_scale_pos < m_scaling; x_scale_pos++) {

						unsigned pos = ((y * m_scaling) + y_scale) * (m_width * m_scaling)
							+ (x * m_scaling) + x_scale_pos;

						dest[pos] =
							(a << 24 | r << 16 | g << 8 | b);
					}
				}
			}
		}
	}

	void Display::SetPixels(unsigned* dest) {
		for (unsigned i = 0; i < m_height; i++) {
			for (unsigned j = 0; j < m_width; j++) {
				ScaleAndSetPixel(i, j, dest);
			}
		}
	}

	void Display::Loop() {
		bool quit = false;

		SDL_Event sdlevent;

		while (!quit) {
			if (m_stop.load()) {
				quit = true;
			}
			else {
				while (SDL_PollEvent(&sdlevent)) {
					if (sdlevent.type == SDL_QUIT) {
						quit = true;
						Stop();
					}
					else
						ProcessEvent(&sdlevent);
				}

				if (m_frame_present.load()) {
					Render();
				}


				SDL_Delay(1);
			}
		}

		if (m_texture) {
			SDL_DestroyTexture(m_texture);
		}

		if (m_renderer) {
			SDL_DestroyRenderer(m_renderer);
		}

		if (m_window) {
			SDL_DestroyWindow(m_window);
		}

		SDL_Quit();
	}

	void Display::Render() {
		std::scoped_lock<std::mutex> lock(m_buffer_mutex);

		SDL_RenderClear(m_renderer);

		void* pixel_ptr;
		int pitch;

		SDL_LockTexture(m_texture, NULL, &pixel_ptr,
			&pitch);

		SetPixels((unsigned*)pixel_ptr);

		SDL_UnlockTexture(m_texture);

		SDL_RenderCopy(m_renderer, m_texture, NULL, NULL);

		SDL_RenderPresent(m_renderer);

		m_frame_present.store(false);
	}

	void Display::SetFrame(byte* buffer) {
		std::scoped_lock<std::mutex> lock(m_buffer_mutex);

		for (unsigned index = 0;
			index < (m_width * m_height); index++) {
			byte value = buffer[index];

			m_pixel_buffer[index * 4] = value;
			m_pixel_buffer[index * 4 + 1] = value;
			m_pixel_buffer[index * 4 + 2] = value;
			m_pixel_buffer[index * 4 + 3] = 0x00;
		}
	}

	void Display::KeyboardDown(SDL_KeyboardEvent* ev) {
		if (m_joypad) {
			switch (ev->keysym.sym)
			{
			case SDLK_RETURN: {
				m_joypad->SetStart();
			} break;

			case SDLK_a: {
				m_joypad->SetA();
			} break;

			case SDLK_w: {
				m_joypad->SetB();
			} break;

			case SDLK_e: {
				m_joypad->SetSelect();
			} break;

			case SDLK_DOWN: {
				m_joypad->SetDown();
			} break;

			case SDLK_UP: {
				m_joypad->SetUp();
			} break;

			case SDLK_LEFT: {
				m_joypad->SetLeft();
			} break;

			case SDLK_RIGHT: {
				m_joypad->SetRight();
			} break;

			case SDLK_LCTRL:
			case SDLK_RCTRL: {
				m_ctrl_status = true;
			} break;

			case SDLK_c: {
				if (m_ctrl_status)
					m_ctrl_c_fun();
			} break;

			default:
				break;
			}
		}
	}

	void Display::KeyboardUp(SDL_KeyboardEvent* ev) {
		if (m_joypad) {
			switch (ev->keysym.sym)
			{
			case SDLK_RETURN: {
				m_joypad->UnsetStart();
			} break;

			case SDLK_a: {
				m_joypad->UnsetA();
			} break;

			case SDLK_w: {
				m_joypad->UnsetB();
			} break;

			case SDLK_e: {
				m_joypad->UnsetSelect();
			} break;

			case SDLK_DOWN: {
				m_joypad->UnsetDown();
			} break;

			case SDLK_UP: {
				m_joypad->UnsetUp();
			} break;

			case SDLK_LEFT: {
				m_joypad->UnsetLeft();
			} break;

			case SDLK_RIGHT: {
				m_joypad->UnsetRight();
			} break;

			case SDLK_LCTRL:
			case SDLK_RCTRL: {
				m_ctrl_status = false;
			} break;

			default:
				break;
			}
		}
	}

	void Display::ProcessEvent(SDL_Event* ev) {
		switch (ev->type)
		{
		case SDL_KEYDOWN: {
			KeyboardDown(&ev->key);
		} break; 

		case SDL_KEYUP: {
			KeyboardUp(&ev->key);
		} break;

		default:
			break;
		}
	}

	bool Display::IsStop() {
		return m_stop.load();
	}

	void Display::Stop() {
		m_stop.store(true);
	}

	Display::~Display() {
		m_stop.store(true);

		m_render_thread.join();
	}

	void Display::SetJoypad(Input::Joypad* joypad) {
		m_joypad = joypad;
	}
}