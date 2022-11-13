#include "../../include/cpu/Cpu.h"

#include <algorithm>
#include <signal.h>

#include "../../include/cpu/Disasm.h"

#include "../../include/cpu/CpuInstr.h"
#include "../../include/state/EmulatorState.h"
#include "../../include/memory/Memory.h"

/*
* Fetch - Decode - Execute
* 
* The fetch consits of a read, 
* for a total of 4 clock cycles (1m cycle),
* the decode and execute can be immediate,
* and, in that case, there are no more
* clock cycles.
* 
* Every read and write uses 1 m cycle
* 
* The first idea was to synchronize 
* external components after each instruction.
* Unfortunately, some opcodes require a lot
* of clock cycles (sometimes even 24), and a 
* lot of things can change during that period.
* 
* EXAMPLE
* 
* CALL d16
* 
* First, we fetch the opcode : 4 clock cycles
* We decode the opcode : 0 cycles
* We need to fetch the data, 2 bytes : 8 cycles
* We need to write the old PC to the stack : 8 cyles
* Write PC ? : 4 cycles
* 
* For this reason, we should sync after every read/write,
* passing the emulator context to each instruction,
* which will call the sync method when needed.
*/

namespace GameboyEmu {
	namespace CPU {

		void Cpu::fillTable() {
			init_cb_instructions();

			m_jumpTable[0x00] = INSTRUCTION_PTR(00);
			m_jumpTable[0xC3] = INSTRUCTION_PTR(C3);
			m_jumpTable[0xAF] = INSTRUCTION_PTR(AF);
			m_jumpTable[0x21] = INSTRUCTION_PTR(21);
			m_jumpTable[0xF3] = INSTRUCTION_PTR(F3);
			m_jumpTable[0x31] = INSTRUCTION_PTR(31);
			m_jumpTable[0xEA] = INSTRUCTION_PTR(EA);
			m_jumpTable[0x3E] = INSTRUCTION_PTR(3E);
			m_jumpTable[0xE0] = INSTRUCTION_PTR(E0);
			m_jumpTable[0xCD] = INSTRUCTION_PTR(CD);
			m_jumpTable[0x7D] = INSTRUCTION_PTR(7D);
			m_jumpTable[0x7C] = INSTRUCTION_PTR(7C);
			m_jumpTable[0x18] = INSTRUCTION_PTR(18);
			m_jumpTable[0xC9] = INSTRUCTION_PTR(C9);
			m_jumpTable[0xE5] = INSTRUCTION_PTR(E5);
			m_jumpTable[0xE1] = INSTRUCTION_PTR(E1);
			m_jumpTable[0xF5] = INSTRUCTION_PTR(F5);
			m_jumpTable[0x23] = INSTRUCTION_PTR(23);
			m_jumpTable[0x2A] = INSTRUCTION_PTR(2A);
			m_jumpTable[0xF1] = INSTRUCTION_PTR(F1);
			m_jumpTable[0xC5] = INSTRUCTION_PTR(C5);
			m_jumpTable[0x01] = INSTRUCTION_PTR(01);
			m_jumpTable[0x03] = INSTRUCTION_PTR(03);
			m_jumpTable[0x78] = INSTRUCTION_PTR(78);
			m_jumpTable[0xB1] = INSTRUCTION_PTR(B1);
			m_jumpTable[0x28] = INSTRUCTION_PTR(28);
			m_jumpTable[0xF0] = INSTRUCTION_PTR(F0);
			m_jumpTable[0xFE] = INSTRUCTION_PTR(FE);
			m_jumpTable[0x20] = INSTRUCTION_PTR(20);
			m_jumpTable[0xC1] = INSTRUCTION_PTR(C1);
			m_jumpTable[0x32] = INSTRUCTION_PTR(32);
			m_jumpTable[0xCB] = INSTRUCTION_PTR(CB);
			m_jumpTable[0x0E] = INSTRUCTION_PTR(0E);
			m_jumpTable[0xE2] = INSTRUCTION_PTR(E2);
			m_jumpTable[0x0C] = INSTRUCTION_PTR(0C);
			m_jumpTable[0x77] = INSTRUCTION_PTR(77);
			m_jumpTable[0x11] = INSTRUCTION_PTR(11);
			m_jumpTable[0x1A] = INSTRUCTION_PTR(1A);
			m_jumpTable[0x13] = INSTRUCTION_PTR(13);
			m_jumpTable[0x7B] = INSTRUCTION_PTR(7B);
			m_jumpTable[0x4F] = INSTRUCTION_PTR(4F);
			m_jumpTable[0x06] = INSTRUCTION_PTR(06);
			m_jumpTable[0x17] = INSTRUCTION_PTR(17);
			m_jumpTable[0x05] = INSTRUCTION_PTR(05);
			m_jumpTable[0x22] = INSTRUCTION_PTR(22);
			m_jumpTable[0x3D] = INSTRUCTION_PTR(3D);
			m_jumpTable[0x0D] = INSTRUCTION_PTR(0D);
			m_jumpTable[0x2E] = INSTRUCTION_PTR(2E);
			m_jumpTable[0x67] = INSTRUCTION_PTR(67);
			m_jumpTable[0x57] = INSTRUCTION_PTR(57);
			m_jumpTable[0x04] = INSTRUCTION_PTR(04);
			m_jumpTable[0x1E] = INSTRUCTION_PTR(1E);
			m_jumpTable[0x1D] = INSTRUCTION_PTR(1D);
			m_jumpTable[0x24] = INSTRUCTION_PTR(24);
			m_jumpTable[0x90] = INSTRUCTION_PTR(90);
			m_jumpTable[0x15] = INSTRUCTION_PTR(15);
			m_jumpTable[0x16] = INSTRUCTION_PTR(16);
			m_jumpTable[0xBE] = INSTRUCTION_PTR(BE);
			m_jumpTable[0x86] = INSTRUCTION_PTR(86);
			m_jumpTable[0xFA] = INSTRUCTION_PTR(FA);
			m_jumpTable[0xE6] = INSTRUCTION_PTR(E6);
			m_jumpTable[0xC4] = INSTRUCTION_PTR(C4);
			m_jumpTable[0x2C] = INSTRUCTION_PTR(2C);
			m_jumpTable[0xA9] = INSTRUCTION_PTR(A9);
			m_jumpTable[0xC6] = INSTRUCTION_PTR(C6);
			m_jumpTable[0xD6] = INSTRUCTION_PTR(D6);
			m_jumpTable[0xB7] = INSTRUCTION_PTR(B7);
			m_jumpTable[0xD5] = INSTRUCTION_PTR(D5);
			m_jumpTable[0x2D] = INSTRUCTION_PTR(2D);
			m_jumpTable[0xAE] = INSTRUCTION_PTR(AE);
			m_jumpTable[0x46] = INSTRUCTION_PTR(46);
			m_jumpTable[0x4E] = INSTRUCTION_PTR(4E);
			m_jumpTable[0x56] = INSTRUCTION_PTR(56);
			m_jumpTable[0x26] = INSTRUCTION_PTR(26);
			m_jumpTable[0x1F] = INSTRUCTION_PTR(1F);
			m_jumpTable[0x30] = INSTRUCTION_PTR(30);
			m_jumpTable[0x25] = INSTRUCTION_PTR(25);
			m_jumpTable[0x5F] = INSTRUCTION_PTR(5F);
			m_jumpTable[0xEE] = INSTRUCTION_PTR(EE);
			m_jumpTable[0x47] = INSTRUCTION_PTR(47);
			m_jumpTable[0x79] = INSTRUCTION_PTR(79);
			m_jumpTable[0x7A] = INSTRUCTION_PTR(7A);
			m_jumpTable[0x72] = INSTRUCTION_PTR(72);
			m_jumpTable[0x71] = INSTRUCTION_PTR(71);
			m_jumpTable[0x70] = INSTRUCTION_PTR(70);
			m_jumpTable[0xD1] = INSTRUCTION_PTR(D1);
			m_jumpTable[0xCE] = INSTRUCTION_PTR(CE);
			m_jumpTable[0xD0] = INSTRUCTION_PTR(D0);
			m_jumpTable[0xC0] = INSTRUCTION_PTR(C0);
			m_jumpTable[0xC8] = INSTRUCTION_PTR(C8);
			m_jumpTable[0xD8] = INSTRUCTION_PTR(D8);
			m_jumpTable[0xB6] = INSTRUCTION_PTR(B6);
			m_jumpTable[0x35] = INSTRUCTION_PTR(35);
			m_jumpTable[0x6E] = INSTRUCTION_PTR(6E);
			m_jumpTable[0x6F] = INSTRUCTION_PTR(6F);
			m_jumpTable[0x29] = INSTRUCTION_PTR(29);
			m_jumpTable[0xE9] = INSTRUCTION_PTR(E9);
			m_jumpTable[0x3C] = INSTRUCTION_PTR(3C);
			m_jumpTable[0xF6] = INSTRUCTION_PTR(F6);
			m_jumpTable[0xC2] = INSTRUCTION_PTR(C2);
			m_jumpTable[0x91] = INSTRUCTION_PTR(91);
			m_jumpTable[0x81] = INSTRUCTION_PTR(81);
			m_jumpTable[0xF8] = INSTRUCTION_PTR(F8);
			m_jumpTable[0x62] = INSTRUCTION_PTR(62);
			m_jumpTable[0x6B] = INSTRUCTION_PTR(6B);
			m_jumpTable[0x12] = INSTRUCTION_PTR(12);
			m_jumpTable[0x1C] = INSTRUCTION_PTR(1C);
			m_jumpTable[0x14] = INSTRUCTION_PTR(14);
			m_jumpTable[0xBB] = INSTRUCTION_PTR(BB);
			m_jumpTable[0x7E] = INSTRUCTION_PTR(7E);
			m_jumpTable[0x38] = INSTRUCTION_PTR(38);
			m_jumpTable[0xF9] = INSTRUCTION_PTR(F9);
			m_jumpTable[0x27] = INSTRUCTION_PTR(27);
			m_jumpTable[0x2F] = INSTRUCTION_PTR(2F);
			m_jumpTable[0xBA] = INSTRUCTION_PTR(BA);
			m_jumpTable[0xB9] = INSTRUCTION_PTR(B9);
			m_jumpTable[0xB8] = INSTRUCTION_PTR(B8);
			m_jumpTable[0xFB] = INSTRUCTION_PTR(FB);
			m_jumpTable[0xCA] = INSTRUCTION_PTR(CA);
			m_jumpTable[0x76] = INSTRUCTION_PTR(76);
			m_jumpTable[0x07] = INSTRUCTION_PTR(07);
			m_jumpTable[0x5D] = INSTRUCTION_PTR(5D);
			m_jumpTable[0x73] = INSTRUCTION_PTR(73);
			m_jumpTable[0x5E] = INSTRUCTION_PTR(5E);
			m_jumpTable[0x08] = INSTRUCTION_PTR(08);
			m_jumpTable[0x66] = INSTRUCTION_PTR(66);
			m_jumpTable[0x33] = INSTRUCTION_PTR(33);
			m_jumpTable[0xAD] = INSTRUCTION_PTR(AD);
			m_jumpTable[0xB0] = INSTRUCTION_PTR(B0);
			m_jumpTable[0x3B] = INSTRUCTION_PTR(3B);
			m_jumpTable[0x39] = INSTRUCTION_PTR(39);
			m_jumpTable[0xE8] = INSTRUCTION_PTR(E8);
			m_jumpTable[0x36] = INSTRUCTION_PTR(36);
			m_jumpTable[0xDE] = INSTRUCTION_PTR(DE);
			m_jumpTable[0x0B] = INSTRUCTION_PTR(0B);
			m_jumpTable[0x1B] = INSTRUCTION_PTR(1B);
			m_jumpTable[0x2B] = INSTRUCTION_PTR(2B);
			m_jumpTable[0x09] = INSTRUCTION_PTR(09);
			m_jumpTable[0x19] = INSTRUCTION_PTR(19);
			m_jumpTable[0x40] = INSTRUCTION_PTR(40);
			m_jumpTable[0x41] = INSTRUCTION_PTR(41);
			m_jumpTable[0x42] = INSTRUCTION_PTR(42);
			m_jumpTable[0x43] = INSTRUCTION_PTR(43);
			m_jumpTable[0x44] = INSTRUCTION_PTR(44);
			m_jumpTable[0x45] = INSTRUCTION_PTR(45);
			m_jumpTable[0x48] = INSTRUCTION_PTR(48);
			m_jumpTable[0x49] = INSTRUCTION_PTR(49);
			m_jumpTable[0x4A] = INSTRUCTION_PTR(4A);
			m_jumpTable[0x4B] = INSTRUCTION_PTR(4B);
			m_jumpTable[0x4C] = INSTRUCTION_PTR(4C);
			m_jumpTable[0x4D] = INSTRUCTION_PTR(4D);
			m_jumpTable[0x50] = INSTRUCTION_PTR(50);
			m_jumpTable[0x51] = INSTRUCTION_PTR(51);
			m_jumpTable[0x52] = INSTRUCTION_PTR(52);
			m_jumpTable[0x53] = INSTRUCTION_PTR(53);
			m_jumpTable[0x54] = INSTRUCTION_PTR(54);
			m_jumpTable[0x55] = INSTRUCTION_PTR(55);
			m_jumpTable[0x58] = INSTRUCTION_PTR(58);
			m_jumpTable[0x59] = INSTRUCTION_PTR(59);
			m_jumpTable[0x5A] = INSTRUCTION_PTR(5A);
			m_jumpTable[0x5B] = INSTRUCTION_PTR(5B);
			m_jumpTable[0x5C] = INSTRUCTION_PTR(5C);
			m_jumpTable[0x60] = INSTRUCTION_PTR(60);
			m_jumpTable[0x61] = INSTRUCTION_PTR(61);
			m_jumpTable[0x63] = INSTRUCTION_PTR(63);
			m_jumpTable[0x64] = INSTRUCTION_PTR(64);
			m_jumpTable[0x65] = INSTRUCTION_PTR(65);
			m_jumpTable[0x68] = INSTRUCTION_PTR(68);
			m_jumpTable[0x69] = INSTRUCTION_PTR(69);
			m_jumpTable[0x6A] = INSTRUCTION_PTR(6A);
			m_jumpTable[0x6C] = INSTRUCTION_PTR(6C);
			m_jumpTable[0x6D] = INSTRUCTION_PTR(6D);
			m_jumpTable[0x74] = INSTRUCTION_PTR(74);
			m_jumpTable[0x75] = INSTRUCTION_PTR(75);
			m_jumpTable[0x7F] = INSTRUCTION_PTR(7F);
			m_jumpTable[0xD2] = INSTRUCTION_PTR(D2);
			m_jumpTable[0xDA] = INSTRUCTION_PTR(DA);
			m_jumpTable[0xCC] = INSTRUCTION_PTR(CC);
			m_jumpTable[0xD4] = INSTRUCTION_PTR(D4);
			m_jumpTable[0xDC] = INSTRUCTION_PTR(DC);
			m_jumpTable[0xD9] = INSTRUCTION_PTR(D9);
			m_jumpTable[0xC7] = INSTRUCTION_PTR(C7);
			m_jumpTable[0xD7] = INSTRUCTION_PTR(D7);
			m_jumpTable[0xE7] = INSTRUCTION_PTR(E7);
			m_jumpTable[0xF7] = INSTRUCTION_PTR(F7);
			m_jumpTable[0xCF] = INSTRUCTION_PTR(CF);
			m_jumpTable[0xDF] = INSTRUCTION_PTR(DF);
			m_jumpTable[0xEF] = INSTRUCTION_PTR(EF);
			m_jumpTable[0xFF] = INSTRUCTION_PTR(FF);
			m_jumpTable[0xF2] = INSTRUCTION_PTR(F2);
			m_jumpTable[0x37] = INSTRUCTION_PTR(37);
			m_jumpTable[0x3F] = INSTRUCTION_PTR(3F);
			m_jumpTable[0xB2] = INSTRUCTION_PTR(B2);
			m_jumpTable[0xB3] = INSTRUCTION_PTR(B3);
			m_jumpTable[0xB4] = INSTRUCTION_PTR(B4);
			m_jumpTable[0xB5] = INSTRUCTION_PTR(B5);
			m_jumpTable[0xBC] = INSTRUCTION_PTR(BC);
			m_jumpTable[0xBD] = INSTRUCTION_PTR(BD);
			m_jumpTable[0xBF] = INSTRUCTION_PTR(BF);
			m_jumpTable[0x80] = INSTRUCTION_PTR(80);
			m_jumpTable[0x82] = INSTRUCTION_PTR(82);
			m_jumpTable[0x83] = INSTRUCTION_PTR(83);
			m_jumpTable[0x84] = INSTRUCTION_PTR(84);
			m_jumpTable[0x85] = INSTRUCTION_PTR(85);
			m_jumpTable[0x87] = INSTRUCTION_PTR(87);
			m_jumpTable[0x88] = INSTRUCTION_PTR(88);
			m_jumpTable[0x89] = INSTRUCTION_PTR(89);
			m_jumpTable[0x8A] = INSTRUCTION_PTR(8A);
			m_jumpTable[0x8B] = INSTRUCTION_PTR(8B);
			m_jumpTable[0x8C] = INSTRUCTION_PTR(8C);
			m_jumpTable[0x8D] = INSTRUCTION_PTR(8D);
			m_jumpTable[0x8F] = INSTRUCTION_PTR(8F);
			m_jumpTable[0x92] = INSTRUCTION_PTR(92);
			m_jumpTable[0x93] = INSTRUCTION_PTR(93);
			m_jumpTable[0x94] = INSTRUCTION_PTR(94);
			m_jumpTable[0x95] = INSTRUCTION_PTR(95);
			m_jumpTable[0x96] = INSTRUCTION_PTR(96);
			m_jumpTable[0x97] = INSTRUCTION_PTR(97);
			m_jumpTable[0x98] = INSTRUCTION_PTR(98);
			m_jumpTable[0x99] = INSTRUCTION_PTR(99);
			m_jumpTable[0x9A] = INSTRUCTION_PTR(9A);
			m_jumpTable[0x9B] = INSTRUCTION_PTR(9B);
			m_jumpTable[0x9C] = INSTRUCTION_PTR(9C);
			m_jumpTable[0x9D] = INSTRUCTION_PTR(9D);
			m_jumpTable[0x9E] = INSTRUCTION_PTR(9E);
			m_jumpTable[0x9F] = INSTRUCTION_PTR(9F);
			m_jumpTable[0xA0] = INSTRUCTION_PTR(A0);
			m_jumpTable[0xA1] = INSTRUCTION_PTR(A1);
			m_jumpTable[0xA2] = INSTRUCTION_PTR(A2);
			m_jumpTable[0xA3] = INSTRUCTION_PTR(A3);
			m_jumpTable[0xA4] = INSTRUCTION_PTR(A4);
			m_jumpTable[0xA5] = INSTRUCTION_PTR(A5);
			m_jumpTable[0xA6] = INSTRUCTION_PTR(A6);
			m_jumpTable[0xA7] = INSTRUCTION_PTR(A7);
			m_jumpTable[0xA8] = INSTRUCTION_PTR(A8);
			m_jumpTable[0xAA] = INSTRUCTION_PTR(AA);
			m_jumpTable[0xAB] = INSTRUCTION_PTR(AB);
			m_jumpTable[0xAC] = INSTRUCTION_PTR(AC);
			m_jumpTable[0x0F] = INSTRUCTION_PTR(0F);
			m_jumpTable[0x0A] = INSTRUCTION_PTR(0A);
			m_jumpTable[0x02] = INSTRUCTION_PTR(02);
			m_jumpTable[0x3A] = INSTRUCTION_PTR(3A);
			m_jumpTable[0x8E] = INSTRUCTION_PTR(8E);
			m_jumpTable[0x34] = INSTRUCTION_PTR(34);
		}

		Cpu::Cpu(State::EmulatorState* emuctx, Mem::Memory* mmu)
			: m_ctx(), m_state(emuctx), m_mem(mmu), m_jumpTable(nullptr)
		{
			m_jumpTable = new jmp_type[256];

			std::fill_n(m_jumpTable, 256, nullptr);

			if (!m_mem->IsBootEnabled())
				m_ctx.ip = 0x100;

			fillTable();
		}

		byte Cpu::handle_interrupts() {
			if (!m_ctx.enableInt)
				return 0;

			//read interrupt request register
			byte ir = m_mem->Read(0xFF0F);
			byte ie = m_mem->Read(0xFFFF);

			if (
				VBLANK_BIT_GET(ir) &&
				VBLANK_BIT_GET(ie)
				) {
				//LOG_INFO(state->getLogger(), "VBLANK Interrupt\n");
				return serve_interrupt(0x00);
			}

			if (
				LCDSTAT_BIT_GET(ir) &&
				LCDSTAT_BIT_GET(ie)
				) {
				//LOG_INFO(state->getLogger(), "LCDSTAT Interrupt\n");
				return serve_interrupt(0x01);
			}

			if (
				TIMER_BIT_GET(ir) &&
				TIMER_BIT_GET(ie)
				) {
				//LOG_INFO(state->getLogger(), "Timer Interrupt\n");
				return serve_interrupt(0x02);
			}

			if (
				SERIAL_BIT_GET(ir) &&
				SERIAL_BIT_GET(ie)
				) {
				//LOG_INFO(state->getLogger(), "Serial Interrupt\n");
				return serve_interrupt(0x03);
			}

			if (
				JOYPAD_BIT_GET(ir) &&
				JOYPAD_BIT_GET(ie)
				) {
				//LOG_INFO(state->getLogger(), "Joypad Interrupt\n");
				return serve_interrupt(0x04);
			}

			return 0;
		}

		byte Cpu::serve_interrupt(byte type) {
			//LOG_INFO(state->getLogger(), "Serving interrupt 0x{2:x}\n", type);

			byte ir = m_mem->Read(0xFF0F);

			m_ctx.enableInt = false;

			word handler = 0x00;

			switch (type)
			{
			case 0x00: //VBLANK
				VBLANK_BIT_RESET(ir);
				handler = 0x40;
				break;

			case 0x01: //LCD STAT
				LCDSTAT_BIT_RESET(ir);
				handler = 0x48;
				break;

			case 0x02: //Timer
				TIMER_BIT_RESET(ir);
				handler = 0x50;
				break;

			case 0x03: //Serial
				SERIAL_BIT_RESET(ir);
				handler = 0x58;
				break;

			case 0x04: //Joypad
				JOYPAD_BIT_RESET(ir);
				handler = 0x60;
				break;

			default:
				LOG_ERR(m_state->GetLogger(), "Invalid interrupt code\n");
				return 0;
			}

			m_mem->Write(0xFF0F, ir);

			interrupt_routine(handler);

			return 5;
		}

		void Cpu::interrupt_routine(word address) {
			m_state->Sync(2);

			m_mem->Write(m_ctx.sp - 1, GET_HIGH(m_ctx.ip));
			m_state->Sync(1);

			m_mem->Write(m_ctx.sp - 2, GET_LOW(m_ctx.ip));
			m_state->Sync(1);

			m_ctx.sp -= 2;

			m_ctx.ip = address;

			m_state->Sync(1);
		}

		byte Cpu::Step() {
			if (m_ctx.halted) {
				byte ie = m_mem->Read(0xFFFF);
				byte ir = m_mem->Read(0xFF0F);

				if ((ir & ie) != 0) {
					m_ctx.halted = false;
				}

				m_state->Sync(1);

				if (m_ctx.ei_delay > 0) {
					m_ctx.ei_delay--;

					m_ctx.enableInt = (m_ctx.ei_delay == 0);
				}

				return 4;
			}

			handle_interrupts();

			byte instruction = m_mem->Read(m_ctx.ip);

			if (!m_ctx.haltBug) {
				m_ctx.ip++;
			}
			else {
				m_ctx.haltBug = false;
			}

			m_state->Sync(1);

			byte cycles = 0;

			//state->getLogger().log_info("Instruction : {0} at 0x{1:x}\n", disassemble(ctx.ip - 1, this->mem).first, ctx.ip - 1);

			if (!m_jumpTable[instruction]) {
				LOG_ERR(m_state->GetLogger(),
					" Unimplemented instruction : 0x{2:x} at 0x{3:x}\n",
					instruction, m_ctx.ip - 1);
			}
			else {
				cycles = (m_jumpTable[instruction])(m_ctx, m_mem, m_state);
			}
			
			if (m_ctx.ei_delay > 0) {
				m_ctx.ei_delay--;

				m_ctx.enableInt = (m_ctx.ei_delay == 0);
			}

			return cycles;
		}

		void Cpu::RunFor(int cycles) {

			while (cycles > 0) {
				cycles -= Step();
			}

		}

		Cpu::~Cpu() {
			delete[] m_jumpTable;
		}

		word Cpu::GetIP() const {
			return m_ctx.ip;
		}

		void Cpu::ResetIP() {
			m_ctx.ip = 0x00;
		}

		CpuContext const& Cpu::GetContext() const {
			return m_ctx;
		}

		bool Cpu::GetIME() const {
			return m_ctx.enableInt;
		}

		std::size_t Cpu::DumpState(byte* buffer, std::size_t offset) {
			WriteWord(buffer, offset, m_ctx.ip);
			WriteWord(buffer, offset + 2, m_ctx.af);
			WriteWord(buffer, offset + 4, m_ctx.bc);
			WriteWord(buffer, offset + 6, m_ctx.de);
			WriteWord(buffer, offset + 8, m_ctx.hl);
			WriteWord(buffer, offset + 10, m_ctx.sp);
			buffer[offset + 12] = m_ctx.enableInt;
			buffer[offset + 13] = m_ctx.ei_delay;
			buffer[offset + 14] = m_ctx.halted;
			buffer[offset + 15] = m_ctx.haltBug;

			return offset + 16;
		}

		std::size_t Cpu::LoadState(byte* buffer, std::size_t offset) {
			m_ctx.ip = ReadWord(buffer, offset);
			m_ctx.af = ReadWord(buffer, offset + 2);
			m_ctx.bc = ReadWord(buffer, offset + 4);
			m_ctx.de = ReadWord(buffer, offset + 6);
			m_ctx.hl = ReadWord(buffer, offset + 8);
			m_ctx.sp = ReadWord(buffer, offset + 10);
			m_ctx.enableInt = buffer[offset + 12];
			m_ctx.ei_delay = buffer[offset + 13];
			m_ctx.halted = buffer[offset + 14];
			m_ctx.haltBug = buffer[offset + 15];
			
			return offset + 16;
		}
	}
}