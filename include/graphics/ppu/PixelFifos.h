#pragma once

#include "../../common/Common.h"
#include "PPU.h"
#include "PixelQueue.h"

namespace GameboyEmu::Mem {
	class Memory;
}

namespace GameboyEmu::Graphics {

	
	struct oam_entry;

	/*
	* THere are 5 modes for both fetchers:
	* - Get tile
	* - Get tile data low
	* - Get tile data high
	* - Sleep
	* - Push
	*
	* The first 4 steps require 2 dots each,
	* the last is attempted every dot
	*/

	class PixelPipeline {
	private:
		static constexpr uint16_t fifo_size = 320;

		PixelQueue<BgPixel, fifo_size> m_fifo;
		PixelQueue<BgPixel, fifo_size> m_temp_bg;
		PixelQueue<SpritePixel, fifo_size> m_sprite_fifo;

		byte m_len;
		byte m_mode;
		byte m_x;
		byte m_popped_off_x;
		bool m_sprite_drawn;
		byte m_spritex;
		byte m_y;

		byte m_mode_dots;

		Mem::Memory* m_mem;
		PPU* m_ppu;

		byte m_wy_trigger;

		byte m_window_line;

		/*
		* Step 1 data
		*/
		word m_map_start;
		word m_tilemap; //tilemap from which tile indexes are taken
		byte m_fetcherX;
		byte m_fetcherY;

		word m_tile_index;

		/*
		* Step 2 data
		*/
		byte m_data_low;

		/*
		* Step 3 data
		*/
		byte m_data_high;

		byte m_over_window;

		byte m_tile_line;

		///  SPRITES/////////
		oam_object* m_sprites;
		byte m_line_sprites;

		byte m_sprite_fetch_status;
		byte m_sprite_fetch_dots;

		byte m_drawn_sprites;

	public:
		PixelPipeline(Mem::Memory* mmu, PPU* ppu);

		~PixelPipeline();

		void Reset(byte y_coord,
			byte wy_trigger,
			byte window_line,
			oam_object* sprites,
			byte num_sprites);

		byte Advance();
		std::pair<byte, bool> AdvanceFifo();

		byte Popfifo();

		bool Fifoempty() const;

		byte GetX();

		void SetSpritePtr(oam_object* ptr);

		std::size_t DumpState(byte* buffer, std::size_t offset);
		std::size_t LoadState(byte* buffer, std::size_t offset);

	private:
		void sprite_fetch();
		void reset_step_1();
	};
}