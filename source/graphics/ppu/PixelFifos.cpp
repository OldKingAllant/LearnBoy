#include "../../../include/graphics/ppu/PixelFifos.h"

#include "../../../include/memory/Memory.h"
#include "../../../include/graphics/ppu/OamEntry.h"

#include <algorithm>

namespace GameboyEmu::Graphics {

	PixelPipeline::PixelPipeline(Mem::Memory* mmu, PPU* ppu) :
		m_fifo(), m_sprite_fifo(), m_len(0), m_mode(0),
		m_x(0), m_popped_off_x(0), m_sprite_drawn(0), m_spritex(0), 
		m_y(0), m_mode_dots(0), m_mem(mmu), m_ppu(ppu),
		m_wy_trigger(0), m_window_line(0), m_map_start(0),
		m_tilemap(0), m_fetcherX(0), m_fetcherY(0), m_tile_index(0),
		m_data_low(0), m_data_high(0),
		m_over_window(0), m_tile_line(0),
		m_sprites(nullptr), m_line_sprites(0),
		m_sprite_fetch_status(0), m_sprite_fetch_dots(0),
		m_drawn_sprites(0)
	{}

	PixelPipeline::~PixelPipeline() {}

	void PixelPipeline::reset_step_1() {
		m_tilemap = 0;
		m_fetcherX = 0;
		m_fetcherY = 0;
		m_map_start = 0;
	}

	std::pair<byte, bool> PixelPipeline::AdvanceFifo() {
		if (!m_fifo.empty() && m_popped_off_x < m_x) {
			BgPixel ret = m_fifo.front();
			SpritePixel sp = m_sprite_fifo.front();

			if (!m_ppu->GetPriority()) {
				ret = BgPixel{ 0x00, true, 0 };
			}

			ret = mix_pixels(ret, sp);

			byte palette_id = ret.palette;

			m_fifo.pop();
			m_sprite_fifo.pop();

			byte mapped =
				ret.blank ? 0xFF : m_ppu->bgColorTranslation(ret.color, palette_id);

			m_popped_off_x = 0;

			return std::pair(mapped
				, true);
		}

		return std::pair(0x00, false);
	}

	byte PixelPipeline::Popfifo() {
		BgPixel ret = m_fifo.front();
		SpritePixel sp = m_sprite_fifo.front();

		if (!m_ppu->GetPriority()) {
			ret = BgPixel{ 0x00, true, 0 };
		}

		ret = mix_pixels(ret, sp);

		byte palette_id = ret.palette;

		m_fifo.pop();
		m_sprite_fifo.pop();

		return ret.blank ?
			0xFF : m_ppu->bgColorTranslation(ret.color, palette_id);
	}

	bool PixelPipeline::Fifoempty() const {
		return m_fifo.empty();
	}

	byte reverseBits(byte num) {
		int start = 7;
		byte res = 0;

		while (start >= 0) {
			res |= ((num >> start) & 1) << (7 - start);
			start--;
		}

		return res;
	}

	void PixelPipeline::sprite_fetch() {
		m_sprite_fetch_dots++;

		if (m_sprite_fetch_dots == 6) {
			oam_object const& sprite = m_sprites[m_drawn_sprites];

			short pos = sprite.x_pos;
			short y_pos = sprite.y_pos;

			byte toshift = 0;
			byte line_start = 0;

			if (pos < 0) {
				toshift = (byte)(-pos);
			}

			byte current_line = 0;

			if (y_pos < 0) {
				line_start = (byte)(-y_pos);
				current_line = m_y + line_start;
			}
			else {
				line_start = (byte)y_pos;
				current_line = m_y - line_start;
			}

			byte tile_1_index = sprite.tile_index_1;
			byte tile_2_index = 0;

			word tile_adress = 0x8000;

			if (sprite.size == 16 || m_ppu->GetObjSize()) {
				tile_1_index &= 0xFE;
				tile_2_index = sprite.tile_index_1 | 1;

				word tile2_address = tile_adress + tile_2_index * 16;
				word tile1_address = tile_adress + tile_1_index * 16;

				word other = 0;

				if (current_line > 7) {
					tile_adress = tile2_address;
					other = tile1_address;

					current_line -= 8;
				}
				else {
					tile_adress = tile1_address;
					other = tile2_address;
				}

				if (sprite.y_flip) {
					tile_adress = other;
				}
			}
			else {
				tile_adress += tile_1_index * 16;
			}

			if (sprite.y_flip) {
				current_line = 7 - current_line;
			}

			tile_adress += (current_line * 2);

			byte low = m_mem->ppu_read_vram(tile_adress);
			byte high = m_mem->ppu_read_vram(tile_adress + 1);

			if (sprite.x_flip) {
				low = reverseBits(low);
				high = reverseBits(high);
			}

			m_spritex += m_sprite_fifo.push_sprite_pixels(
				low, high, m_sprites[m_drawn_sprites],
				toshift
			);

			m_sprite_fetch_dots = 0;
			m_sprite_fetch_status = 0;
			m_drawn_sprites++;

			m_sprite_drawn = true;
		}
	}

	byte PixelPipeline::Advance() {
		if (m_sprite_fetch_status != 0) {
			sprite_fetch();

			if (m_sprite_fetch_status != 0)
				return m_x;
		}

		/*if (!ppu->getPriority()) {
			if (mode == 0x04) {
				for (int i = 0; i < 8; i++) {
					fifo.push(BgPixel{ 0, true, 0 });
					sprite_fifo.push(SpritePixel{
							0, 0, 0, 0, true
						});
				}

				x += 8;
				spritex += 8;

				mode_dots = 0;
				mode = 0x00;
			}
			else {
				mode_dots++;

				if (mode_dots == 2) {
					mode++;
					mode_dots = 0;
				}
			}

			return x;
		}*/

		switch (m_mode)
		{
		case 0x00:
		{
			if (m_mode_dots == 0) {
				/*
				* Check if we are over the window
				* (this flag is reset only the next
				* scanline)
				*/
				if (m_over_window) {
					//Get position of tile map
					m_map_start = m_ppu->GetWindowMap();

					m_map_start = m_map_start ? 0x9C00 :
						0x9800;

					//WX is the starting X pos - 7
					m_fetcherX = ((m_x - (m_ppu->GetWX() - 7)) & 0xFF) / 8;
					m_fetcherY = m_window_line / 8;

					//line in the current tile
					m_tile_line = m_window_line % 8;
				}
				else {
					m_map_start = m_ppu->GetBGMap();

					m_map_start = m_map_start ? 0x9C00 :
						0x9800;

					m_fetcherX = (((word)m_ppu->GetSCX() +
						(word)m_x) >> 3) & 31;

					m_fetcherY = ((word)m_ppu->GetSCY() + (word)m_y) & 255;

					/*
					* The real formula to calculate
					* the correct line in the tile
					* would be (SCY + y) - SCY,
					* but it is equivalent
					* to SCY - SCY + y which is y
					*/
					m_tile_line = m_fetcherY % 8;

					m_fetcherY /= 8;
				}
			}
			else {
				/*
				* The tilemap contains the
				* index of the tile in the
				* tile data range
				*
				* The tilemap consists of
				* a table of 32  * 32 entries
				*
				* Map base + (row * 32) + column
				*/
				m_tilemap = m_map_start + (m_fetcherY * 32)
					+ m_fetcherX;
			}

			m_mode_dots++;

			if (m_mode_dots == 2) {
				m_mode_dots = 0;
				m_mode = 0x01;
				m_data_low = 0x00;
			}
			//Get tile

		} break;

		case 0x01:
		{
			m_mode_dots++;

			//0 - 8800
			//1 - 8000

			if (m_mode_dots == 2) {
				/*
				* The tile offset is given by the index
				* found in the tilemap * the size in bytes
				* of a tile
				*/
				word index = m_mem->ppu_read_vram(m_tilemap);
				byte index_copy = (byte)index;

				index *= 16;

				byte addressing_mode = m_ppu->GetAddressingMode();
				//byte addressing_mode = 1;
				if (addressing_mode) {
					m_tile_index = 0x8000 +
						index + (m_tile_line * 2);

					//8000 Base pointer
					m_data_low = m_mem->ppu_read_vram(m_tile_index);
				}
				else {
					//9000 Base pointer, signed index
					//signed short sindex = (signed short)index;

					short sindex = static_cast<signed char>(index_copy) + 128;
					sindex *= 16;

					m_tile_index = (word)(0x8800 + sindex)
						+ (m_tile_line * 2);
					m_data_low = m_mem->ppu_read_vram(m_tile_index);
				}

				m_mode_dots = 0;
				m_data_high = 0x00;
				m_mode = 0x02;
			}
		} break;

		case 0x02:
		{
			m_mode_dots++;

			//0 - 8800
			//1 - 8000

			if (m_mode_dots == 2) {
				m_tile_index++;

				m_data_high = m_mem->ppu_read_vram(m_tile_index);

				m_mode_dots = 0;
				m_mode = 0x03;
			}
		} break;

		case 0x03:
		{
			//Idle even if the fifo
			//is already empty
			m_mode_dots++;

			if (m_mode_dots == 2) {
				m_mode_dots = 0;
				m_mode = 0x04;
			}
		} break;

		case 0x04:
		{
			if (!m_sprite_drawn) {
				//Push pixels
				PixelQueue<BgPixel, 16> temp;

				/*
				* Most significant bit is the
				* leftmost pixel
				*/
				for (int i = 7; i >= 0; i--) {
					byte bit1 = GET_BIT(m_data_low, i);
					byte bit2 = GET_BIT(m_data_high, i);

					byte color = (bit2 << 1) | bit1;

					temp.push(BgPixel{ color, false });
				}

				/*
				* Pixels are shifted only from the
				* first tile. Subsequent pixels
				* will be pushed in the correct
				* position
				*/
				if (m_x == 0 && !m_over_window) {
					int toshift = m_ppu->GetSCX() % 8;

					while (toshift > 0)
					{
						temp.pop();
						toshift--;
					}
				}

				m_temp_bg.push(temp);
			}
			else {
				m_sprite_drawn = false;
			}


			bool obj_encountered = false;

			while (m_temp_bg.size() > 0 && m_x < 160 && !obj_encountered) {
				if (m_wy_trigger) {
					if (
						!m_ppu->GetWindowEnable()
						) {
						m_over_window = 0;
					}
					else {
						if (
							(int)m_x >= (int)(m_ppu->GetWX() - 7)
							&& !m_over_window) {
							m_over_window = 1;

							m_temp_bg.reset();
						}
					}
				}

				if (!m_temp_bg.empty()) {
					if (
						m_line_sprites > 0 &&
						m_drawn_sprites < m_line_sprites &&
						(short)m_x >= (short)(m_sprites[m_drawn_sprites].x_pos) &&
						(short)m_x < (short)(m_sprites[m_drawn_sprites].x_pos + 8) &&
						m_ppu->GetObjEnable()) {
						m_sprite_fetch_status = 1;
						m_sprite_fetch_dots = 0;

						obj_encountered = true;
					}
					else {
						m_fifo.push(m_temp_bg.front());

						if (m_spritex <= m_x) {
							m_sprite_fifo.push(SpritePixel{
							0, 0, 0, 0, true, -1
								});
							m_spritex++;
						}

						m_temp_bg.pop();

						m_x++;
					}
				}
			}

			if (!obj_encountered) {
				reset_step_1();

				m_mode = 0x00;
				m_mode_dots = 0;
			}

		} break;

		default:
			break;
		}

		return m_x;
	}

	void PixelPipeline::Reset(byte y_coord,
		byte _wy_trigger,
		byte _win_line,
		oam_object* sprites,
		byte num_sprites) {

		m_sprites = sprites;
		m_line_sprites = num_sprites;

		if (num_sprites > 0) {
			std::sort(sprites, sprites + num_sprites,
				[](oam_object const& o1, oam_object const& o2) {
					if (o1.x_pos == o2.x_pos) {
						return o1.oam_index < o2.oam_index;
					}

					return o1.x_pos < o2.x_pos;
				});
		}

		m_fifo.reset();
		m_sprite_fifo.reset();
		m_temp_bg.reset();

		m_sprite_fetch_status = 0;
		m_sprite_fetch_dots = 0;
		m_drawn_sprites = 0;

		m_len = 0;
		m_mode = 0;
		m_x = 0;
		m_popped_off_x = 0;
		m_sprite_drawn = false;
		m_spritex = 0;

		m_y = y_coord;

		m_mode_dots = 0;

		m_wy_trigger = _wy_trigger;
		m_window_line = _win_line;

		m_over_window = 0;

		reset_step_1();
	}

	byte PixelPipeline::GetX() {
		return m_x;
	}

	std::size_t PixelPipeline::DumpState(byte* buffer, std::size_t offset) {
		WriteWord(buffer, offset, (word)m_fifo.len & 0xFFFF);
		WriteWord(buffer, offset + 2, (word)m_fifo.first & 0xFFFF);
		WriteWord(buffer, offset + 4, (word)m_fifo.last & 0xFFFF);

		offset += 6;

		WriteWord(buffer, offset, (word)m_temp_bg.len & 0xFFFF);
		WriteWord(buffer, offset + 2, (word)m_temp_bg.first & 0xFFFF);
		WriteWord(buffer, offset + 4, (word)m_temp_bg.last & 0xFFFF);

		offset += 6;

		WriteWord(buffer, offset, (word)m_sprite_fifo.len & 0xFFFF);
		WriteWord(buffer, offset + 2, (word)m_sprite_fifo.first & 0xFFFF);
		WriteWord(buffer, offset + 4, (word)m_sprite_fifo.last & 0xFFFF);

		offset += 6;

		for (int i = 0; i < 320; i++) {
			auto const& pixel = m_fifo.pixels[i];

			buffer[offset] = pixel.color;
			buffer[offset + 1] = pixel.blank;
			buffer[offset + 2] = pixel.palette;

			offset += 3;
		}

		for (int i = 0; i < 320; i++) {
			auto const& pixel = m_temp_bg.pixels[i];

			buffer[offset] = pixel.color;
			buffer[offset + 1] = pixel.blank;
			buffer[offset + 2] = pixel.palette;

			offset += 3;
		}

		for (int i = 0; i < 320; i++) {
			auto const& pixel = m_sprite_fifo.pixels[i];

			buffer[offset] = pixel.color;
			buffer[offset + 1] = pixel.palette;
			buffer[offset + 2] = pixel.oam_index;
			buffer[offset + 3] = pixel.priority;
			buffer[offset + 4] = pixel.blank;

			WriteWord(buffer, offset + 5, (word)pixel.x);

			offset += 7;
		}

		buffer[offset] = m_len;
		buffer[offset + 1] = m_mode;
		buffer[offset + 2] = m_x;
		buffer[offset + 3] = m_popped_off_x;
		buffer[offset + 4] = m_sprite_drawn;
		buffer[offset + 5] = m_spritex;
		buffer[offset + 6] = m_y;

		buffer[offset + 7] = m_mode_dots;
		buffer[offset + 8] = m_wy_trigger;
		buffer[offset + 9] = m_window_line;

		WriteWord(buffer, offset + 10, m_map_start);
		WriteWord(buffer, offset + 12, m_tilemap);

		buffer[offset + 14] = m_fetcherX;
		buffer[offset + 15] = m_fetcherY;

		WriteWord(buffer, offset + 17, m_tile_index);

		buffer[offset + 19] = m_data_low;
		buffer[offset + 20] = m_data_high;
		buffer[offset + 21] = m_over_window;
		buffer[offset + 22] = m_tile_line;

		offset += 23;

		for (int i = 0; i < 10; i++) {
			const oam_object& obj = m_sprites[i];

			WriteWord(buffer, offset, (word)obj.y_pos);
			WriteWord(buffer, offset + 2, (word)obj.x_pos);

			buffer[offset + 4] = obj.tile_index_1;
			buffer[offset + 5] = obj.bg_w_over_obj;
			buffer[offset + 6] = obj.y_flip;
			buffer[offset + 7] = obj.x_flip;
			buffer[offset + 8] = obj.palette_num;
			buffer[offset + 9] = obj.oam_index;
			buffer[offset + 10] = obj.size;

			offset += 11;
		}

		buffer[offset] = m_line_sprites;
		buffer[offset + 1] = m_sprite_fetch_status;
		buffer[offset + 2] = m_sprite_fetch_dots;
		buffer[offset + 3] = m_drawn_sprites;

		return offset + 4;
	}

	std::size_t PixelPipeline::LoadState(byte* buffer, std::size_t offset) {
		m_fifo.len = ReadWord(buffer, offset);
		m_fifo.first = ReadWord(buffer, offset + 2);
		m_fifo.last = ReadWord(buffer, offset + 4);

		offset += 6;

		m_temp_bg.len = ReadWord(buffer, offset);
		m_temp_bg.first = ReadWord(buffer, offset + 2);
		m_temp_bg.last = ReadWord(buffer, offset + 4);

		offset += 6;

		m_sprite_fifo.len = ReadWord(buffer, offset);
		m_sprite_fifo.first = ReadWord(buffer, offset + 2);
		m_sprite_fifo.last = ReadWord(buffer, offset + 4);

		offset += 6;
		
		for (int i = 0; i < 320; i++) {
			auto& pixel = m_fifo.pixels[i];

			pixel.color = buffer[offset];
			pixel.blank = buffer[offset + 1];
			pixel.palette = buffer[offset + 2];

			offset += 3;
		}

		for (int i = 0; i < 320; i++) {
			auto& pixel = m_temp_bg.pixels[i];

			pixel.color = buffer[offset];
			pixel.blank = buffer[offset + 1];
			pixel.palette = buffer[offset + 2];

			offset += 3;
		}

		for (int i = 0; i < 320; i++) {
			auto& pixel = m_sprite_fifo.pixels[i];

			pixel.color = buffer[offset];
			pixel.palette = buffer[offset + 1];
			pixel.oam_index = buffer[offset + 2];
			pixel.priority = buffer[offset + 3];
			pixel.blank = buffer[offset + 4];

			pixel.x = (short)ReadWord(buffer, offset + 5);

			offset += 7;
		}

		m_len = buffer[offset];
		m_mode = buffer[offset + 1];
		m_x = buffer[offset + 2];
		m_popped_off_x = buffer[offset + 3];
		m_sprite_drawn = buffer[offset + 4];
		m_spritex = buffer[offset + 5];
		m_y = buffer[offset + 6];

		m_mode_dots = buffer[offset + 7];
		m_wy_trigger = buffer[offset + 8];
		m_window_line = buffer[offset + 9];

		m_map_start = ReadWord(buffer, offset + 10);
		m_tilemap = ReadWord(buffer, offset + 12);

		m_fetcherX = buffer[offset + 14];
		m_fetcherY = buffer[offset + 15];

		m_tile_index = ReadWord(buffer, offset + 17);

		m_data_low = buffer[offset + 19];
		m_data_high = buffer[offset + 20];
		m_over_window = buffer[offset + 21];
		m_tile_line = buffer[offset + 22];

		offset += 23;

		for (int i = 0; i < 10; i++) {
			oam_object& obj = m_sprites[i];

			obj.y_pos = (short)ReadWord(buffer, offset);
			obj.x_pos = (short)ReadWord(buffer, offset + 2);

			obj.tile_index_1 = buffer[offset + 4];
			obj.bg_w_over_obj = buffer[offset + 5];
			obj.y_flip = buffer[offset + 6];
			obj.x_flip = buffer[offset + 7];
			obj.palette_num = buffer[offset + 8];
			obj.oam_index = buffer[offset + 9];
			obj.size = buffer[offset + 10];

			offset += 11;
		}

		m_line_sprites = buffer[offset];
		m_sprite_fetch_status = buffer[offset + 1];
		m_sprite_fetch_dots = buffer[offset + 2];
		m_drawn_sprites = buffer[offset + 3];

		return offset + 4;
	}

	void PixelPipeline::SetSpritePtr(oam_object* ptr) {
		m_sprites = ptr;
	}
}