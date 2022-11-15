#pragma once

#include "../../common/Common.h"
#include "OamEntry.h"

namespace GameboyEmu::Graphics {

	struct BgPixel {
		byte color;
		bool blank = false;
		byte palette = 0;
	};

	struct SpritePixel {
		byte color;
		byte palette;
		byte oam_index;
		byte priority;
		bool blank = false;
		short x = 0;
	};

	template <typename PixelType, size_t Len>
	class PixelQueue {
	public:
		PixelType pixels[Len];

		size_t len;

		size_t first;
		size_t last;

	public:
		PixelQueue();

		PixelQueue(PixelQueue<PixelType, Len> const&);

		PixelQueue(PixelQueue<PixelType, Len>&&);

		bool empty() const;

		size_t size() const;

		void reset();
		void clear();

		PixelType const& front() const;
		PixelType& front();

		PixelType const& back() const;
		PixelType back();

		void push(PixelType const& pixel);
		void pop();

		byte push_sprite_pixels(byte low,
			byte high, oam_object const& obj, byte ignore);

		template <size_t Len2>
		void push(PixelQueue<PixelType, Len2> const& other) {
			for (byte i = 0; i < other.len; i++) {
				pixels[++last] = other.pixels[other.first + i];
			}

			len += other.len;
		}
	};


	template <typename PixelType, size_t Len>
	PixelQueue<PixelType, Len>::PixelQueue() : pixels{}, len(0), first(0), last(-1) {}

	template <typename PixelType, size_t Len>
	PixelQueue<PixelType, Len>::PixelQueue(PixelQueue const& other) : PixelQueue() {
		std::copy_n(other.pixels, other.len, pixels);

		len = other.len;

		first = 0;
		last = len - 1;
	}

	template <typename PixelType, size_t Len>
	PixelQueue<PixelType, Len>::PixelQueue(PixelQueue&& other) : PixelQueue() {
		std::copy_n(other.pixels, other.len, pixels);

		len = other.len;

		first = 0;
		last = len - 1;
	}

	template <typename PixelType, size_t Len>
	bool PixelQueue<PixelType, Len>::empty() const {
		return len == 0;
	}

	template <typename PixelType, size_t Len>
	size_t PixelQueue<PixelType, Len>::size() const {
		return len;
	}

	template <typename PixelType, size_t Len>
	void PixelQueue<PixelType, Len>::reset() {
		len = 0;

		first = 0;
		last = -1;
	}

	template <typename PixelType, size_t Len>
	void PixelQueue<PixelType, Len>::clear() {
		std::fill_n(pixels, Len, PixelType{});

		len = 0;
		first = 0;
		last = -1;
	}

	template <typename PixelType, size_t Len>
	PixelType const& PixelQueue<PixelType, Len>::front() const {
		return pixels[first];
	}

	template <typename PixelType, size_t Len>
	PixelType& PixelQueue<PixelType, Len>::front() {
		return pixels[first];
	}

	template <typename PixelType, size_t Len>
	PixelType const& PixelQueue<PixelType, Len>::back() const {
		return pixels[last];
	}

	template <typename PixelType, size_t Len>
	PixelType PixelQueue<PixelType, Len>::back() {
		return pixels[last];
	}

	template <typename PixelType, size_t Len>
	void PixelQueue<PixelType, Len>::push(PixelType const& pixel) {
		pixels[++last] = pixel;

		len++;
	}

	template <typename PixelType, size_t Len>
	void PixelQueue<PixelType, Len>::pop() {
		_ASSERT(len != 0);

		if (len == 0)
			return;

		first++;
		len--;
	}

	template <typename PixelType, size_t Len>
	byte PixelQueue<PixelType, Len>::push_sprite_pixels(byte low,
		byte high, oam_object const& obj, byte ignore) {
		int curr_pixel = 7 - ignore;

		if (!empty()) {
			for (unsigned i = (unsigned)first; i <= last && curr_pixel >= 0; i++) {
				if (pixels[i].x == obj.x_pos) {
					if (pixels[i].oam_index > obj.oam_index || pixels[i].blank) {
						byte bit1 = GET_BIT(low, curr_pixel);
						byte bit2 = GET_BIT(high, curr_pixel);

						byte color = (bit2 << 1) | bit1;

						pixels[i].color = color;
						pixels[i].palette = obj.palette_num;
						pixels[i].oam_index = obj.oam_index;
						pixels[i].priority = obj.bg_w_over_obj;

						pixels[i].blank = false;
						pixels[i].x = obj.x_pos;
					}

					curr_pixel--;
				}
				else if (pixels[i].x < obj.x_pos) {
					if (pixels[i].blank || pixels[i].color == 0) {
						byte bit1 = GET_BIT(low, curr_pixel);
						byte bit2 = GET_BIT(high, curr_pixel);

						byte color = (bit2 << 1) | bit1;

						pixels[i].color = color;
						pixels[i].palette = obj.palette_num;
						pixels[i].oam_index = obj.oam_index;
						pixels[i].priority = obj.bg_w_over_obj;

						pixels[i].blank = false;
						pixels[i].x = obj.x_pos;
					}
					curr_pixel--;
				}
			}
		}

		byte pushed_pixels = 0;

		for (int i = curr_pixel; i >= 0; i--, pushed_pixels++) {
			byte bit1 = GET_BIT(low, i);
			byte bit2 = GET_BIT(high, i);

			byte color = (bit2 << 1) | bit1;

			pixels[++last] = SpritePixel{
				color,
				obj.palette_num,
				obj.oam_index,
				obj.bg_w_over_obj,
				false,
				obj.x_pos
			};

			len++;
		}

		return pushed_pixels;
	}

	BgPixel mix_pixels(BgPixel const&
		bg, SpritePixel const& spritepx);

}