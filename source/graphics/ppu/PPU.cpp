#include "../../../include/graphics/ppu/PPU.h"
#include "../../../include/memory/Memory.h"
#include "../../../include/state/EmulatorState.h"
#include "../../../include/graphics/ppu/PixelFifos.h"

namespace GameboyEmu::Graphics {

	PPU::PPU(State::EmulatorState* state) :
		m_state(state), m_mem(nullptr),
		m_ctx(), m_objects(nullptr), m_current_object_fetch{},
		m_fetch_count(0), m_encountered_objs(0),
		m_oam_index(0), m_wy_trigger(0),
		m_current_scanline_cycles(0), m_pipeline(nullptr),
		m_frame(nullptr), m_pixel_index(0) {
		m_objects = new oam_object[10];

		DisableLcd();

		m_ctx.enable = 0x00;

		m_frame = new byte[160 * 144];

		m_window_line = 255;
	}

	byte PPU::GetWindowMap() const {
		return m_ctx.window_tile_map;
	}

	byte PPU::GetBGMap() const {
		return m_ctx.bg_tile_map;
	}

	void PPU::SetMemory(Mem::Memory* mmu) {
		m_mem = mmu;

		m_pipeline = new PixelPipeline(mmu, this);
	}

	void PPU::stat_source(byte type) {
		bool doit = false;

		switch (type)
		{
		case 0x00:
			doit = m_ctx.stat_source_ly_lyc;
			break;

		case 0x01:
			doit = m_ctx.stat_source_oam;
			break;

		case 0x02:
			doit = m_ctx.stat_source_vblank;
			break;

		case 0x03:
			doit = m_ctx.stat_source_hblank;
			break;

		default:
			break;
		}

		if (doit) {
			byte ir = m_mem->Read(0xFF0F);

			LCDSTAT_BIT_SET(ir);

			m_mem->Write(0xFF0F, ir);
		}
	}

	void PPU::reset_oam_scan() {
		std::fill_n(m_objects, 10, oam_object{});

		m_current_object_fetch[0] = 0x00;
		m_current_object_fetch[1] = 0x00;
		m_current_object_fetch[2] = 0x00;
		m_current_object_fetch[3] = 0x00;

		m_fetch_count = 0;
		m_encountered_objs = 0;
		m_oam_index = 0;

		m_wy_trigger = 0;
	}

	byte PPU::bgColorTranslation(byte id, byte palette_id) const {
		constexpr byte mapping[4] = { 0xFF, 0x7F, 0x3F, 0x00 };

		byte palette = m_ctx.bg_palette;

		if (palette_id == 1) {
			palette = m_ctx.obj0_palette;
		}
		else if (palette_id == 2) {
			palette = m_ctx.obj1_palette;
		}

		byte index = 0;

		switch (id)
		{
		case 0x00: {
			index = (GET_BIT(palette, 1) << 1) | GET_BIT(palette, 0);
		} break;

		case 0x01: {
			index = (GET_BIT(palette, 3) << 1) | GET_BIT(palette, 2);
		} break;

		case 0x02: {
			index = (GET_BIT(palette, 5) << 1) | GET_BIT(palette, 4);
		} break;

		case 0x03: {
			index = (GET_BIT(palette, 7) << 1) | GET_BIT(palette, 6);
		} break;
		default:
			break;
		}

		return mapping[index & 0b11];
	}

	byte PPU::GetObjEnable() const {
		return m_ctx.obj_enable;
	}

	bool window_Visible(byte wx, byte wy, byte ly) {
		return (wy < 143)
			&& (wx < 166)
			&& (ly >= wy);
	}

	void PPU::checkWyTrigger() {
		if (m_ctx.lcd_y >= m_ctx.wy && m_ctx.window_enable) {
			m_wy_trigger = 1;

			if (window_Visible(m_ctx.wx, m_ctx.wy, m_ctx.lcd_y)) {
				m_window_line++;
			}
		}
		else
			m_wy_trigger = 0;
	}

	void PPU::Tick(byte cycles) {
		if (!m_ctx.enable)
			return;

		word tstates = cycles * 4;

		while (tstates > 0) {
			switch (m_ctx.mode_flag)
			{
			case 0x00: // HBLANK
			{
				tstates--;
				m_current_scanline_cycles++;

				mode_hblank();
			} break;

			case 0x01: // VBLANK
			{
				tstates--;
				m_current_scanline_cycles++;

				mode_vblank();
			} break;

			case 0x02: // OAM scan
			{
				mode_oam();

				tstates--;
			} break;

			case 0x03: // Pixel output
			{

				/*m_pipeline->Advance();

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
				}*/
				mode_transfer();

				tstates--;
			} break;

			default:
				break;
			}
		}
	}

	PPU::~PPU() {
		delete[] m_objects;
		delete[] m_frame;
		delete m_pipeline;
	}

	byte PPU::GetLCD_Control() const {
		return (m_ctx.enable << 7) |
			(m_ctx.window_tile_map << 6) |
			(m_ctx.window_enable << 5) |
			(m_ctx.bg_window_tile_data << 4) |
			(m_ctx.bg_tile_map << 3) |
			(m_ctx.obj_size << 2) |
			(m_ctx.obj_enable << 1) |
			m_ctx.bg_window_en_priority;
	}

	void PPU::SetLCD_Control(byte value) {
		byte old_value = m_ctx.enable;

		m_ctx.enable = GET_BIT(value, 7);
		m_ctx.window_tile_map = GET_BIT(value, 6);
		m_ctx.window_enable = GET_BIT(value, 5);
		m_ctx.bg_window_tile_data = GET_BIT(value, 4);
		m_ctx.bg_tile_map = GET_BIT(value, 3);
		m_ctx.obj_size = GET_BIT(value, 2);
		m_ctx.obj_enable = GET_BIT(value, 1);
		m_ctx.bg_window_en_priority = value & 1;

		if (!old_value && m_ctx.enable) {
			EnableLcd();
		}
		else if (old_value && !m_ctx.enable) {
			DisableLcd();
		}
	}

	void PPU::EnableLcd() {
		//LOG_INFO(m_state->GetLogger(),
			//"Enabling LCD\n");

		m_ctx.lcd_y = 0;

		reset_oam_scan();

		m_current_scanline_cycles = 0;

		m_ctx.mode_flag = 2;
		m_ctx.lyc_ly_flag = 0;

		m_pixel_index = 0;

		m_window_line = 255;
	}

	void PPU::DisableLcd() {
		//LOG_INFO(m_state->GetLogger(),
			//"Disabling LCD\n");

		m_ctx.lcd_y = 0;

		reset_oam_scan();

		m_current_scanline_cycles = 0;

		m_ctx.mode_flag = 0;
		m_ctx.lyc_ly_flag = 0;

		m_pixel_index = 0;

		m_window_line = 255;
	}

	byte PPU::GetLCD_Status() const {
		return (m_ctx.stat_source_ly_lyc << 6) |
			(m_ctx.stat_source_oam << 5) |
			(m_ctx.stat_source_vblank << 4) |
			(m_ctx.stat_source_hblank << 3) |
			(m_ctx.lyc_ly_flag << 2) |
			(m_ctx.mode_flag & 0b11);
	}

	void PPU::SetLCD_Status(byte value) {
		//LOG_INFO(m_state->GetLogger(),
			//"Setting LCD Status : {2:b}\n", value);

		m_ctx.stat_source_ly_lyc = GET_BIT(value, 6);
		m_ctx.stat_source_oam = GET_BIT(value, 5);
		m_ctx.stat_source_vblank = GET_BIT(value, 4);
		m_ctx.stat_source_hblank = GET_BIT(value, 3);
	}

	byte PPU::GetEnabled() const {
		return m_ctx.enable;
	}

	byte PPU::GetWindowTileMap() const {
		return m_ctx.window_tile_map;
	}

	byte PPU::GetWindowEnable() const {
		return m_ctx.window_enable;
	}

	byte PPU::GetWBGTileData() const {
		return m_ctx.bg_window_tile_data;

	}
	byte PPU::GetBgTileMap() const {
		return m_ctx.bg_tile_map;
	}

	byte PPU::GetObjSize() const {
		return m_ctx.obj_size;
	}

	byte PPU::GetPriority() const {
		return m_ctx.bg_window_en_priority;
	}

	byte PPU::GetStatSourceLyc() const {
		return m_ctx.stat_source_ly_lyc;
	}

	byte PPU::GetStatSourceOam() const {
		return m_ctx.stat_source_oam;
	}

	byte PPU::GetStatSourceVblank() const {
		return m_ctx.stat_source_vblank;
	}

	byte PPU::GetStatSourceHblank() const {
		return m_ctx.stat_source_hblank;
	}

	byte PPU::GetLycFlag() const {
		return m_ctx.lyc_ly_flag;
	}

	byte PPU::GetMode() const {
		return m_ctx.mode_flag;
	}



	void PPU::SetSCX(byte value) {
		m_ctx.scx = value;
	}

	byte PPU::GetSCX() const {
		return m_ctx.scx;
	}

	void PPU::SetSCY(byte value) {
		m_ctx.scy = value;
	}

	byte PPU::GetSCY() const {
		return m_ctx.scy;
	}



	void PPU::SetWX(byte value) {
		m_ctx.wx = value;
	}

	byte PPU::GetWX() const {
		return m_ctx.wx;
	}

	void PPU::SetWY(byte value) {
		m_ctx.wy = value;
	}

	byte PPU::GetWY() const {
		return m_ctx.wy;
	}



	byte PPU::GetScanline() const {
		return m_ctx.lcd_y;
	}



	byte PPU::GetLYC() const {
		return m_ctx.lyc;
	}

	void PPU::SetLYC(byte value) {
		m_ctx.lyc = value;
	}



	byte PPU::GetBGPalette() const {
		return m_ctx.bg_palette;
	}

	void PPU::SetBGPalette(byte value) {
		m_ctx.bg_palette = value;
	}



	byte PPU::GetOBJ0Palette() const {
		return m_ctx.obj0_palette;
	}

	byte PPU::GetOBJ1Palette() const {
		return m_ctx.obj1_palette;
	}

	void PPU::SetOBJ0Palette(byte value) {
		m_ctx.obj0_palette = value;
	}

	void PPU::SetOBJ1Palette(byte value) {
		m_ctx.obj1_palette = value;
	}

	byte PPU::GetAddressingMode() const {
		return m_ctx.bg_window_tile_data;
	}

	//Yes, we could just interpret the struct
	//as a series of chars, but
	//i cannot be sure that the alignment
	//of the object is the same across
	//all the compilers
	std::size_t PPU::DumpState(byte* buffer, std::size_t offset) {
		buffer[offset] = m_ctx.enable;
		buffer[offset + 1] = m_ctx.window_tile_map;
		buffer[offset + 2] = m_ctx.window_enable;
		buffer[offset + 3] = m_ctx.bg_window_tile_data;
		buffer[offset + 4] = m_ctx.bg_tile_map;
		buffer[offset + 5] = m_ctx.obj_size;
		buffer[offset + 6] = m_ctx.obj_enable;
		buffer[offset + 7] = m_ctx.bg_window_en_priority;

		buffer[offset + 8] = m_ctx.stat_source_ly_lyc;
		buffer[offset + 9] = m_ctx.stat_source_oam;
		buffer[offset + 10] = m_ctx.stat_source_vblank;
		buffer[offset + 11] = m_ctx.stat_source_hblank;
		buffer[offset + 12] = m_ctx.lyc_ly_flag;
		buffer[offset + 13] = m_ctx.mode_flag;

		buffer[offset + 14] = m_ctx.scx;
		buffer[offset + 15] = m_ctx.scy;

		buffer[offset + 16] = m_ctx.wx;
		buffer[offset + 17] = m_ctx.wy;

		buffer[offset + 18] = m_ctx.lcd_y;
		buffer[offset + 19] = m_ctx.lyc;

		buffer[offset + 20] = m_ctx.bg_palette;
		buffer[offset + 21] = m_ctx.obj0_palette;
		buffer[offset + 22] = m_ctx.obj1_palette;

		offset += 23;

		for (int i = 0; i < 10; i++) {
			const oam_object& obj = m_objects[i];

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

		buffer[offset] = m_current_object_fetch[0];
		buffer[offset + 1] = m_current_object_fetch[1];
		buffer[offset + 2] = m_current_object_fetch[2];
		buffer[offset + 3] = m_current_object_fetch[3];

		buffer[offset + 4] = m_fetch_count;
		buffer[offset + 5] = m_encountered_objs;
		buffer[offset + 6] = m_oam_index;
		buffer[offset + 7] = m_wy_trigger;
		buffer[offset + 8] = m_window_line;

		WriteWord(buffer, offset + 9, m_current_scanline_cycles);

		WriteWord(buffer, offset + 11, (word)(m_pixel_index & 0xFFFF));
		WriteWord(buffer, offset + 13, (word)((m_pixel_index >> 16) & 0xFFFF));

		offset += 15;

		std::copy_n(m_frame, 160 * 144, buffer + offset);

		offset += (160 * 144);

		offset = m_pipeline->DumpState(buffer, offset);

		return offset;
	}

	std::size_t PPU::LoadState(byte* buffer, std::size_t offset) {
		m_ctx.enable = buffer[offset];
		m_ctx.window_tile_map = buffer[offset + 1];
		m_ctx.window_enable = buffer[offset + 2];
		m_ctx.bg_window_tile_data = buffer[offset + 3];
		m_ctx.bg_tile_map = buffer[offset + 4];
		m_ctx.obj_size = buffer[offset + 5];
		m_ctx.obj_enable = buffer[offset + 6];
		m_ctx.bg_window_en_priority = buffer[offset + 7];

		m_ctx.stat_source_ly_lyc = buffer[offset + 8];
		m_ctx.stat_source_oam = buffer[offset + 9];
		m_ctx.stat_source_vblank = buffer[offset + 10];
		m_ctx.stat_source_hblank = buffer[offset + 11];
		m_ctx.lyc_ly_flag = buffer[offset + 12];
		m_ctx.mode_flag = buffer[offset + 13];

		m_ctx.scx = buffer[offset + 14];
		m_ctx.scy = buffer[offset + 15];

		m_ctx.wx = buffer[offset + 16];
		m_ctx.wy = buffer[offset + 17];

		m_ctx.lcd_y = buffer[offset + 18];
		m_ctx.lyc = buffer[offset + 19];

		m_ctx.bg_palette = buffer[offset + 20];
		m_ctx.obj0_palette = buffer[offset + 21];
		m_ctx.obj1_palette = buffer[offset + 22];

		offset += 23;

		for (int i = 0; i < 10; i++) {
			oam_object& obj = m_objects[i];

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

		m_current_object_fetch[0] = buffer[offset];
		m_current_object_fetch[1] = buffer[offset + 1];
		m_current_object_fetch[2] = buffer[offset + 2];
		m_current_object_fetch[3] = buffer[offset + 3];

		m_fetch_count = buffer[offset + 4];
		m_encountered_objs = buffer[offset + 5];
		m_oam_index = buffer[offset + 6];
		m_wy_trigger = buffer[offset + 7];
		m_window_line = buffer[offset + 8];

		m_current_scanline_cycles = ReadWord(buffer, offset + 9);

		m_pixel_index = ReadWord(buffer, offset + 11);
		m_pixel_index |= (ReadWord(buffer, offset + 13) << 16);

		offset += 15;

		std::copy_n(buffer + offset, 160 * 144, m_frame);

		offset += (160 * 144);

		m_pipeline->SetSpritePtr(m_objects);

		offset = m_pipeline->LoadState(buffer, offset);

		return offset;
	}
}