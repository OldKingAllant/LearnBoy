#include "../../include/memory/Memory.h"

#include "../../include/state/EmulatorState.h"
#include "../../include/graphics/ppu/PPU.h"
#include "../../include/timing/Timer.h"
#include "../../include/input/Joypad.h"
#include "../../include/sound/apu/APU.h"
#include "../../include/datatransfer/Serial.h"

#include <fstream>
#include <filesystem>

namespace GameboyEmu {
	namespace Mem {


		//This constructor sucks
		Memory::Memory(State::EmulatorState* ctx, Cartridge::MemoryCard* card, Graphics::PPU* pp, Timing::Timer* tim, Input::Joypad* joypad, Sound::APU* apu, DataTransfer::Serial* serial)
			: m_state(ctx), m_cartridge(card), m_ppu(pp), m_timer(tim), m_joypad(joypad), m_apu(apu), m_serial(serial),
			m_bootROMEnabled(false), m_wram(nullptr),
			m_hram(nullptr), m_vram(nullptr), m_oam(nullptr), 
			m_interrupt_enable(0x00), m_interrupt_flag(0x00),
			m_dma(), m_bootrom(nullptr) {
			m_wram = new byte[8 * 1024];
			m_hram = new byte[0xFFFF - 0xFF80];
			m_vram = new byte[8 * 1024];
			m_oam = new byte[0xFE9F - 0xFE00 + 1];

			std::fill_n(m_oam, 0xFE9F - 0xFE00 + 1, 0xFF);

			std::fill_n(m_vram, 8 * 1024, 0x00);

			m_bootROMEnabled = false;
		}

		byte Memory::ApplyShark(Cheats::GameShark const& shark) {
			byte old = 0;

			if (shark.address < 0xC000) {
				//external ram
				return m_cartridge->ApplyShark(shark);
			}
			else {
				old = m_wram[shark.address - 0xC000];

				m_wram[shark.address - 0xC000] = shark.new_data;
			}

			return old;
		}

		void Memory::ReadBootrom(std::string const& path) {
			if (!std::filesystem::exists(path)) {
				LOG_ERR(m_state->GetLogger(), "Fatal: invalid boot rom\n");
				std::exit(0);
			}

			if (!std::filesystem::is_regular_file(path)) {
				LOG_ERR(m_state->GetLogger(), "Invalid boot rom\n");
				std::exit(0);
			}

			//open file in read binary mode
			std::ifstream file(path, std::ios::in | std::ios::binary);

			//file is not readable 
			if (!file.good())
				std::exit(0);

			//set position at the end of the file and read the index value
			file.seekg(0, std::ios::end);

			std::streampos sz = file.tellg();

			//reset the position
			file.seekg(0, std::ios::beg);
			////////////////////////

			//allocate buffer
			byte* data = new byte[sz];

			//copy all the file into memory
			file.read((char*)data, sz);

			m_bootrom = data;

			m_bootROMEnabled = true;
		}

		void Memory::reset_dma() {
			m_dma.running = false;
			m_dma.current_index = 0;
			m_dma.current_oam_index = 0;
			m_dma.source_base = 0x00;
			m_dma.cycle_count = 0;
		}

		void Memory::DmaAdvance(byte cycles) {
			if (!m_dma.running)
				return;

			bool end = false;

			cycles *= 4;

			while (cycles > 0 && !end) {
				byte val = Read(m_dma.source_base +
					m_dma.current_index);

				m_oam[m_dma.current_oam_index] = val;

				cycles--;

				m_dma.current_index++;
				m_dma.current_oam_index++;

				if (m_dma.current_index == 0xA0)
					end = true;
			}

			if (end) {
				reset_dma();
			}
		}

		byte Memory::ppu_read_vram(word address) const {
			return m_vram[address - 0x8000];
		}

		byte Memory::ppu_read_oam(word address) const {
			//if (m_dma.running)
				//return 0xFF;

			return m_oam[address - 0xFE00];
		}

		/*
		* Writes value to address,
		* effect depends on the address
		* range.
		*/
		byte Memory::Read(word address) const {
			//0x8000 - 0x9FFF
			//0xFE00 - 0xFE9F
			if (address <= 0x7FFF) { //reading from ROM
				//if the boot rom is enabled we should read from it
				if (!m_bootROMEnabled || address >= 0x100)
					return m_cartridge->Read(address);
				else
					return m_bootrom[address];
			}
			else if (address >= 0x8000 && address < 0xA000) {
				//VRAM
				/*if (m_ppu->GetMode() == 0x03) {
					LOG_WARN(m_state->GetLogger(),
						"Reading VRAM at address 0x{2:x} during mode 3\n",
						address);
					return 0xFF;
				}*/

				return m_vram[address - 0x8000];
			}
			else if (0xA000 <= address && address <= 0xBFFF) {
				return m_cartridge->Read(address);
			}
			else if (0xC000 <= address && address <= 0xDFFF) {
				return m_wram[address - 0xC000];
			}
			else if (address >= 0xFE00 && address < 0xFEA0) {
				byte mode = m_ppu->GetMode();

				if (mode == 0x03 || mode == 0x02) {
					LOG_WARN(m_state->GetLogger(),
						"Reading OAM at address 0x{2:x} during mode 2 or 3\n",
						address);
					return 0xFF;
				}

				return m_oam[address - 0xFE00];
			}
			else if (address >= 0xFF00 && address < 0xFF80) {
				if (address >= 0xFF10 && address <= 0xFF3F) {
					return m_apu->ReadReg(address);
				}
				
				switch (address)
				{
				case 0xFF00: {
					return m_joypad->Read();
				} break;

				case 0xFF01: {
					return m_serial->ReadData();
				} break;

				case 0xFF02: {
					return m_serial->ReadControl();
				} break;

				case 0xFF04: {
					return m_timer->GetDiv();
				} break;

				case 0xFF05: {
					return m_timer->GetTima();
				} break;

				case 0xFF06: {
					return m_timer->GetTma();
				} break;

				case 0xFF07: {
					return m_timer->GetTAC();
				} break;

				case 0xFF0F: {
					return m_interrupt_flag.load();
				} break;

				case 0xFF40: {
					return m_ppu->GetLCD_Control();
				} break;

				case 0xFF41: {
					return m_ppu->GetLCD_Status();
				} break;

				case 0xFF42: {
					return m_ppu->GetSCY();
				} break;

				case 0xFF43: {
					return m_ppu->GetSCX();
				} break;

				case 0xFF44: {
					return m_ppu->GetScanline();
				} break;

				case 0xFF45: {
					return m_ppu->GetLYC();
				} break;

				case 0xFF46: {
					return (m_dma.source_base & 0xFF00) >> 8;
				} break;

				case 0xFF47: {
					return m_ppu->GetBGPalette();
				} break;

				case 0xFF48: {
					return m_ppu->GetOBJ0Palette();
				} break;

				case 0xFF49: {
					return m_ppu->GetOBJ1Palette();
				} break;

				case 0xFF4A: {
					return m_ppu->GetWY();
				} break;

				case 0xFF4B: {
					return m_ppu->GetWX();
				} break;

				case 0xFF50: {
					return m_bootROMEnabled;
				} break;

				default:
					//LOG_WARN(state->getLogger(),
						//"Invalid or unimplemented I/O register : 0x{2:x}\n", address);
					break;
				}
			}
			else if (address >= 0xFF80 && address < 0xFFFF) {
				return m_hram[address - 0xFF80];
			}
			else if (address == 0xFFFF) {
				return m_interrupt_enable;
			}
			else {
				LOG_WARN(m_state->GetLogger(), " Unimplemented, reading address {2:x}\n", address);
			}

			return 0xFF;
		}

		void Memory::Write(word address, byte value) {
			if (address <= 0x7FFF) {
				m_cartridge->Write(address, value);
			}
			else if (address >= 0x8000 && address < 0xA000) {
				//VRAM
				/*if (m_ppu->GetMode() == 0x03) {
					LOG_WARN(m_state->GetLogger(),
						"Writing VRAM at address 0x{2:x} during mode 3\n",
						address);
					return;
				}*/

				//state->getLogger().log_info(
					//"VRAM[0x{0:x}] = 0x{1:x}\n", address, value);

				m_vram[address - 0x8000] = value;
			}
			else if (0xA000 <= address && address <= 0xBFFF) {
				m_cartridge->Write(address, value);
			}
			else if (0xC000 <= address && address <= 0xDFFF) {
				m_wram[address - 0xC000] = value;
			}
			else if (address >= 0xFE00 && address < 0xFEA0) {
				/*byte mode = m_ppu->GetMode();

				if (mode == 0x03 || mode == 0x02) {
					LOG_WARN(m_state->GetLogger(),
						"Writing OAM at address 0x{2:x} during mode 2 or 3\n",
						address);
					return;
				}*/

				m_oam[address - 0xFE00] = value;
			}
			else if (address >= 0xFF00 && address < 0xFF80) {
				if (address >= 0xFF10 && address <= 0xFF3F) {
					m_apu->WriteReg(address, value);
					return;
				}
				
				switch (address)
				{
				case 0xFF00: {
					m_joypad->Write(value);
				} break;

				case 0xFF01: {
					m_serial->WriteData(value);
				} break;

				case 0xFF02: {
					m_serial->WriteControl(value);
				} break;

				case 0xFF04: {
					m_timer->SetDiv(value);
				} break;

				case 0xFF05: {
					m_timer->SetTima(value);
				} break;

				case 0xFF06: {
					m_timer->SetTma(value);
				} break;

				case 0xFF07: {
					m_timer->SetTAC(value);
				} break;

				case 0xFF0F: {
					m_interrupt_flag.store(value);
				} break;

				case 0xFF40: {
					m_ppu->SetLCD_Control(value);
				}
						   break;

				case 0xFF41: {
					m_ppu->SetLCD_Status(value);
				}
						   break;

				case 0xFF42: {
					m_ppu->SetSCY(value);
				} break;

				case 0xFF43: {
					m_ppu->SetSCX(value);
				} break;

				case 0xFF44: {
					LOG_WARN(m_state->GetLogger(),
						"Trying to write read only register 0xFF44\n");
				} break;

				case 0xFF45: {
					m_ppu->SetLYC(value);
				} break;

				case 0xFF46: {
					reset_dma();

					m_dma.running = true;

					m_dma.source_base = value * 0x100;
				} break;

				case 0xFF47: {
					m_ppu->SetBGPalette(value);
				} break;

				case 0xFF48: {
					m_ppu->SetOBJ0Palette(value);
				} break;

				case 0xFF49: {
					m_ppu->SetOBJ1Palette(value);
				} break;

				case 0xFF4A: {
					m_ppu->SetWY(value);
				} break;

				case 0xFF4B: {
					m_ppu->SetWX(value);
				} break;

				case 0xFF50: {
					if (m_bootROMEnabled) {
						m_bootROMEnabled = !(value != 0);
					}
				} break;

				default:
					//LOG_WARN(state->getLogger(),
						//"Invalid or unimplemented I/O register : 0x{2:x} writing value 0x{3:x}\n"
						//, address, value);
					break;
				}
			}
			else if (address >= 0xFF80 && address < 0xFFFF) {
				m_hram[address - 0xFF80] = value;
			}
			else if (address == 0xFFFF) {
				m_interrupt_enable = (byte)value;
			}
			else {
				LOG_WARN(m_state->GetLogger(), "Unimplemented memory write at {2:x}\n", address);
			}

		}

		Memory::~Memory() {
			LOG_INFO(m_state->GetLogger(), " Destroying memory object\n");

			delete[] m_wram;
			delete[] m_hram;
			delete[] m_vram;
			delete[] m_oam;
		}

		bool Memory::IsBootEnabled() const {
			return m_bootROMEnabled;
		}

		byte Memory::GetIE() const {
			return m_interrupt_enable;
		}

		byte Memory::GetIR() const {
			return m_interrupt_flag;
		}

		dma_status const& Memory::GetDma() const {
			return m_dma;
		}

		std::size_t Memory::DumpState(byte* buffer, std::size_t offset) {
			buffer[offset] = m_dma.running;
			WriteWord(buffer, offset + 1, m_dma.source_base);
			buffer[offset + 3] = m_dma.current_index;
			buffer[offset + 4] = m_dma.current_oam_index;
			buffer[offset + 5] = m_dma.cycle_count;

			buffer[offset + 6] = m_bootROMEnabled;

			offset += 7;

			std::copy_n(m_wram, StaticData::wram_size, buffer + offset);

			offset += StaticData::wram_size;

			std::copy_n(m_hram, StaticData::hram_size, buffer + offset);

			offset += StaticData::hram_size;

			std::copy_n(m_vram, StaticData::vram_size, buffer + offset);

			offset += StaticData::vram_size;

			std::copy_n(m_oam, StaticData::oam_size, buffer + offset);

			offset += StaticData::oam_size;

			buffer[offset] = m_interrupt_enable;
			buffer[offset + 1] = m_interrupt_flag.load();
			
			offset += 2;

			std::copy_n(m_bootrom, StaticData::dmg_bootrom_size, buffer + offset);

			offset += StaticData::dmg_bootrom_size;

			return offset;
		}

		std::size_t Memory::LoadState(byte* buffer, std::size_t offset) {
			m_dma.running = buffer[offset];
			m_dma.source_base = ReadWord(buffer, offset + 1);
			m_dma.current_index = buffer[offset + 3];
			m_dma.current_oam_index = buffer[offset + 4];
			m_dma.cycle_count = buffer[offset + 5];

			m_bootROMEnabled = buffer[offset + 6];

			offset += 7;

			std::copy_n(buffer + offset, StaticData::wram_size, m_wram);

			offset += StaticData::wram_size;

			std::copy_n(buffer + offset, StaticData::hram_size, m_hram);

			offset += StaticData::hram_size;

			std::copy_n(buffer + offset, StaticData::vram_size, m_vram);

			offset += StaticData::vram_size;

			std::copy_n(buffer + offset, StaticData::oam_size, m_oam);

			offset += StaticData::oam_size;

			m_interrupt_enable = buffer[offset];
			m_interrupt_flag = buffer[offset + 1];

			offset += 2;

			std::copy_n(buffer + offset, StaticData::dmg_bootrom_size, m_bootrom);

			offset += StaticData::dmg_bootrom_size;

			return offset;
		}
	}
}