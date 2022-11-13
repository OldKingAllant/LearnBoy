#pragma once

#include "../../common/Common.h"
#include "OamEntry.h"

#define VISIBLE_HEIGHT 144
#define VISIBLE_WIDHT 160

#define HEIGHT 154
#define WIDHT 160

namespace GameboyEmu {
	namespace State {
		class EmulatorState;
	}

	namespace Mem {
		class Memory;
	}

	namespace Graphics {
		class PixelPipeline;
		struct BgPixel;



		class PPU {
		public:
			PPU(State::EmulatorState* state);

		private:
			State::EmulatorState* m_state;
			Mem::Memory* m_mem;

			//8000 -	9FFF	8 KiB Video RAM (VRAM)
			//found in Memory

			//FE00 -	FE9F	Sprite attribute table (OAM)
			//found in Memory

			//FF40	FF4B	
			//DMG	LCD Control, Status, 
			//Position, Scrolling, and Palettes

			struct ppu_context {
				//FF40 LCD Control
				byte enable;
				byte window_tile_map;
				byte window_enable;
				byte bg_window_tile_data;
				byte bg_tile_map;
				byte obj_size;
				byte obj_enable;
				byte bg_window_en_priority;

				//FF41 LCD status
				byte stat_source_ly_lyc;
				byte stat_source_oam;
				byte stat_source_vblank;
				byte stat_source_hblank;
				byte lyc_ly_flag;
				byte mode_flag;

				//SCX SCY
				//Scroll x/y of the background
				byte scx;
				byte scy;

				//WX WY
				//Starting position of the window
				//on the screen
				byte wx;
				byte wy;

				//FF44
				//current scanline
				byte lcd_y;

				//lyc 
				//LY compare
				//FF45
				byte lyc;

				//Palettes
				//FF47 BG palette data
				byte bg_palette;

				//FF48 - FF49 OBJ palette data
				byte obj0_palette;
				byte obj1_palette;
			};

			ppu_context m_ctx;

			oam_object* m_objects;
			byte m_current_object_fetch[4];
			byte m_fetch_count;
			byte m_encountered_objs;
			byte m_oam_index;

			byte m_wy_trigger;
			byte m_window_line;

			word m_current_scanline_cycles;

			PixelPipeline* m_pipeline;

			byte* m_frame;
			unsigned m_pixel_index;

		private:
			void stat_source(byte type);

			void reset_oam_scan();

			void checkWyTrigger();

			void mode_hblank();
			void mode_vblank();
			void mode_oam();
			void mode_transfer();

		public:

			void EnableLcd();
			void DisableLcd();

			void SetMemory(Mem::Memory* mmu);

			~PPU();

			byte GetLCD_Control() const;
			void SetLCD_Control(byte value);

			byte GetEnabled() const;
			byte GetWindowTileMap() const;
			byte GetWindowEnable() const;
			byte GetWBGTileData() const;
			byte GetBgTileMap() const;
			byte GetObjSize() const;
			byte GetObjEnable() const;
			byte GetPriority() const;



			byte GetLCD_Status() const;
			void SetLCD_Status(byte value);

			byte GetStatSourceLyc() const;
			byte GetStatSourceOam() const;
			byte GetStatSourceVblank() const;
			byte GetStatSourceHblank() const;
			byte GetLycFlag() const;
			byte GetMode() const;



			void SetSCX(byte value);
			byte GetSCX() const;

			void SetSCY(byte value);
			byte GetSCY() const;



			void SetWX(byte value);
			byte GetWX() const;

			void SetWY(byte value);
			byte GetWY() const;



			byte GetScanline() const;



			byte GetLYC() const;
			void SetLYC(byte value);



			byte GetBGPalette() const;
			void SetBGPalette(byte value);



			byte GetOBJ0Palette() const;
			byte GetOBJ1Palette() const;

			void SetOBJ0Palette(byte value);
			void SetOBJ1Palette(byte value);

			//Advances the ppu process
			//for mcycles
			void Tick(byte mcycles);

			byte GetWindowMap() const;
			byte GetBGMap() const;

			byte GetAddressingMode() const;

			//
			byte bgColorTranslation(byte id, byte palette) const;

			std::size_t DumpState(byte* buffer, std::size_t offset);
			std::size_t LoadState(byte* buffer, std::size_t offset);
		};
	}
}