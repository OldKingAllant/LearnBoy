#pragma once

#include "../../common/Common.h"

namespace GameboyEmu::Graphics {
	struct oam_object {
		short y_pos; // Y position on the screen + 16
		short x_pos; // X position on the screen + 8
		byte tile_index_1; // Index of first (or last) tile 
		byte bg_w_over_obj; // Priority with BG and Window
		byte y_flip;
		byte x_flip;
		byte palette_num;
		byte oam_index;
		byte size;
	};
}