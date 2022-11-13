#pragma once

#include "../logging/Logger.h"
#include "../cartridge/MemoryCard.h"

#include "../cheats/GameShark.h"

#include <atomic>

/*
* 0x0000 - 0x3FFF First ROM bank
* 0x4000 - 0x7FFF 1 - N Rom bank
* 0x8000 - 0x9FFF Video Ram
* 0xA000 - 0xBFFF External RAM
* 0xC000 - 0xCFFF Work RAM bank 0
* 0xD000 - 0xDFFF Work RAM bank 2
* 0xE000 - 0xFDFF Echo RAM (Mirror of C000-DDFF)
* 0xFE00 - 0xFE9F OAM Sprite Attribute Table
* 0xFEA0 - 0xFEFF Not usable
* 0xFF00 - 0xF7FF I/O
* 0xFF80 - 0xFFFE HRAM
* 0xFFFF - Interrupt Enable 
*/

namespace GameboyEmu {
	namespace State {
		class EmulatorState;
	}

	namespace Graphics {
		class PPU;
	}

	namespace Timing {
		class Timer;
	}

	namespace Input {
		class Joypad;
	}

	namespace Sound {
		class APU;
	}

	namespace DataTransfer {
		class Serial;
	}

	namespace Mem {

		struct dma_status {
			bool running;
			word source_base;
			byte current_index;

			byte current_oam_index;

			byte cycle_count;
		};

		/*
		* Links all the addressable memory
		*/
		class Memory {
		public:
			Memory(State::EmulatorState* ctx, Cartridge::MemoryCard* card,
				Graphics::PPU* pp, Timing::Timer* tim, 
				Input::Joypad* joypad, Sound::APU* apu, 
				DataTransfer::Serial* serial);

			//Read byte from memory
			byte Read(word address) const;

			//Write byte to memory
			void Write(word address, byte value);

			//Returns if the boot rom is still enabled
			bool IsBootEnabled() const;

			byte GetIE() const;
			byte GetIR() const;

			byte ppu_read_vram(word address) const;
			byte ppu_read_oam(word address) const;

			~Memory();

			void DmaAdvance(byte cycles);

			dma_status const& GetDma() const;

			std::size_t DumpState(byte* buffer, std::size_t offset);
			std::size_t LoadState(byte* buffer, std::size_t offset);

			byte ApplyShark(Cheats::GameShark const& shark);

			void ReadBootrom(std::string const& path);

		private:
			State::EmulatorState* m_state;
			Cartridge::MemoryCard* m_cartridge;
			Graphics::PPU* m_ppu;
			Timing::Timer* m_timer;
			Input::Joypad* m_joypad;
			Sound::APU* m_apu;
			DataTransfer::Serial* m_serial;
			bool m_bootROMEnabled;

			byte* m_wram;
			byte* m_hram;
			byte* m_vram;
			byte* m_oam;

			/*
			*
			*	Bit 0: VBlank   Interrupt Enable  (INT $40)  (1=Enable)
				Bit 1: LCD STAT Interrupt Enable  (INT $48)  (1=Enable)
				Bit 2: Timer    Interrupt Enable  (INT $50)  (1=Enable)
				Bit 3: Serial   Interrupt Enable  (INT $58)  (1=Enable)
				Bit 4: Joypad   Interrupt Enable  (INT $60)  (1=Enable)
			*/
			byte m_interrupt_enable;

			std::atomic_uchar m_interrupt_flag;

			dma_status m_dma;

			byte* m_bootrom;

			void reset_dma();
		};
	}
}