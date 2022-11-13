#pragma once

#include "../logging/Logger.h"
#include "../memory/Memory.h"
#include "./CpuContext.h"

namespace GameboyEmu {
	namespace State {
		class EmulatorState;
	}

	namespace Mem {
		class Memory;
	}

	namespace CPU {

		using jmp_type = byte(*)(CPU::CpuContext&, Mem::Memory*, State::EmulatorState*);

		class Cpu {
		public:
			Cpu(State::EmulatorState* ctx, Mem::Memory* mmu);

			/*
			* Runs for the given cycles (probably unused)
			*/
			void RunFor(int cycles);

			/*
			* Executes exactly one instruction,
			* and returns the number of cycles
			*/
			byte Step();

			word GetIP() const;
			void ResetIP();

			bool GetIME() const;

			CpuContext const& GetContext() const;

			~Cpu();

			std::size_t DumpState(byte* buffer, std::size_t offset);
			std::size_t LoadState(byte* buffer, std::size_t offset);

		private:
			//Registers
			CpuContext m_ctx;
			State::EmulatorState* m_state;

			//The addressable bus
			Mem::Memory* m_mem;

			//Instructions
			jmp_type* m_jumpTable;

			//Init jump table
			void fillTable();

			/*
			* Tries to handle interrupts
			*/
			byte handle_interrupts();

			/*
			* Serves one specific interrupt
			*/
			byte serve_interrupt(byte type);

			/*
			* Saves the Instruction Pointer
			* and jumps to the interrupt handler
			*/
			void interrupt_routine(word address);
		};

	}

}