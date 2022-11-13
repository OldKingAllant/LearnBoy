#include "../../../include/graphics/ppu/PPU.h"
#include "../../../include/memory/Memory.h"
#include "../../../include/state/EmulatorState.h"
#include "../../../include/graphics/ppu/PixelFifos.h"

namespace GameboyEmu::Graphics {

	void PPU::mode_hblank() {
		if (m_current_scanline_cycles == 456) {
			m_current_scanline_cycles = 0;
			m_ctx.lcd_y++;

			if (m_ctx.lcd_y == 144) {
				//Enter VBLANK mode
				m_ctx.mode_flag = 0x01;

				stat_source(0x02);

				byte ir = m_mem->Read(0xFF0F);

				VBLANK_BIT_SET(ir);

				m_mem->Write(0xFF0F, ir);

				m_state->ShowFrame(m_frame);

				m_pixel_index = 0;
			}
			else {
				reset_oam_scan();

				//WY is checked only
				//at the beginning
				//of mode 2
				checkWyTrigger();

				m_ctx.mode_flag = 0x02;

				stat_source(0x01);
			}

			if (m_ctx.lcd_y == m_ctx.lyc) {
				m_ctx.lyc_ly_flag = 0x01;

				stat_source(0x00);
			}
			else {
				m_ctx.lyc_ly_flag = 0x00;
			}
		}
	}

	void PPU::mode_vblank() {
		if (m_current_scanline_cycles == 456) {
			m_current_scanline_cycles = 0;
			m_ctx.lcd_y++;

			if (m_ctx.lcd_y == 154) {
				m_ctx.lcd_y = 0;

				reset_oam_scan();

				//WY is checked only
				//at the beginning
				//of mode 2
				checkWyTrigger();

				m_ctx.mode_flag = 0x02;

				stat_source(0x01);
			}

			if (m_ctx.lcd_y == m_ctx.lyc) {
				m_ctx.lyc_ly_flag = 0x01;

				stat_source(0x00);
			}
			else {
				m_ctx.lyc_ly_flag = 0x00;
			}

			m_window_line = 255;
		}
	}

	void PPU::mode_oam() {

		if (m_encountered_objs < 10) {
			m_current_object_fetch[m_fetch_count++]
				= m_mem->ppu_read_oam(
					0xFE00 + m_oam_index);
			m_current_object_fetch[m_fetch_count++]
				= m_mem->ppu_read_oam(
					0xFE00 + m_oam_index + 1);

			if (m_fetch_count == 4) {
				m_fetch_count = 0;

				byte y = m_current_object_fetch[0];
				byte x = m_current_object_fetch[1];
				byte tile_index = m_current_object_fetch[2];
				byte attributes = m_current_object_fetch[3];

				short real_y = y - 16;
				byte h = m_ctx.obj_size ? 16 : 8;


				if (real_y <= (short)m_ctx.lcd_y && real_y + h > (short)m_ctx.lcd_y) {
					//found object
					m_objects[m_encountered_objs].y_pos = real_y;
					m_objects[m_encountered_objs].x_pos = x - 8;
					m_objects[m_encountered_objs].tile_index_1 = tile_index;
					m_objects[m_encountered_objs].oam_index = m_oam_index - 4;
					m_objects[m_encountered_objs].bg_w_over_obj =
						GET_BIT(attributes, 7);
					m_objects[m_encountered_objs].y_flip =
						GET_BIT(attributes, 6);
					m_objects[m_encountered_objs].x_flip =
						GET_BIT(attributes, 5);
					m_objects[m_encountered_objs].palette_num =
						GET_BIT(attributes, 4);
					m_objects[m_encountered_objs].size = h;

					m_encountered_objs++;
				}

				//encountered_objs++;
			}
		}
		else {
			bool here = true;
		}

		m_oam_index += 2;

		m_current_scanline_cycles++;

		if (m_current_scanline_cycles >= 80) {
			m_ctx.mode_flag = 0x03;
			m_pipeline->Reset(
				m_ctx.lcd_y, m_wy_trigger, m_window_line,
				m_objects, m_encountered_objs
			);
		}
	}

	void PPU::mode_transfer() {
		m_pipeline->Advance();

		auto pixel = m_pipeline->AdvanceFifo();

		if (pixel.second) {

			m_frame[m_pixel_index++] = pixel.first;
		}

		m_current_scanline_cycles++;

		if (m_pipeline->GetX() >= 160) {
			while (m_pixel_index % 160 != 0
				&& !m_pipeline->Fifoempty()) {
				m_frame[m_pixel_index++] = m_pipeline->Popfifo();
			}

			//Switch to HBLANK
			m_ctx.mode_flag = 0x00;


			stat_source(0x03);
		}
	}

}