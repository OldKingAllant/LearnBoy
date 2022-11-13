#include "../../../include/graphics/ppu/PixelQueue.h"

namespace GameboyEmu::Graphics {

	BgPixel mix_pixels(BgPixel const&
		bg, SpritePixel const& spritepx) {
		if (bg.blank && spritepx.blank) {
			return BgPixel{ 0x00, true, 0 };
		}

		if (spritepx.blank) {
			return bg;
		}

		if (bg.blank) {
			return BgPixel{ spritepx.color,
							false,
							(byte)(spritepx.palette + 1) };
		}

		if (spritepx.color == 0 || (spritepx.priority && bg.color != 0)) {
			return bg;
		}

		return BgPixel{ spritepx.color, false, (byte)(spritepx.palette + 1) };
	}
}