#pragma once

#include <string_view>
#include <map>
#include <vector>
#include <chrono>
#include <atomic>

#include "../common/Common.h"
#include "../logging/Logger.h"
#include "../debugger/Watchpoint.h"
#include "../debugger/StacktraceEntry.h"

#include "../cheats/GameGenie.h"
#include "../cheats/GameShark.h"

namespace GameboyEmu {
	namespace CPU {
		class Cpu;
	}

	namespace Mem {
		class Memory;
	}

	namespace Cartridge {
		class MemoryCard;
	}

	namespace Graphics {
		class PPU;
		class Display;
	}

	namespace Timing {
		class Timer;
	}

	namespace Input {
		class Joypad;
	}

	namespace Sound {
		class APU;
		class OutputDevice;
	}

	namespace DataTransfer {
		class Serial;
	}

	namespace State {

		/*
		* The emulator class, which contains all the components
		* of the emulator and manages cpu runtime.
		*
		* It also offers a method used for synchronization between
		* the CPU and the other components.
		*/
		class EmulatorState {
		private:
			std::string m_file;

			Logger& m_logger;

			CPU::Cpu* m_cpu;
			Mem::Memory* m_memory;
			Cartridge::MemoryCard* m_card;
			Graphics::PPU* m_ppu;
			Timing::Timer* m_timer;
			Input::Joypad* m_joypad;
			Sound::APU* m_apu;
			DataTransfer::Serial* m_serial;

			bool m_fatal;

			std::string m_fatal_message;

			Graphics::Display* m_display;

			unsigned m_stop_check_cycles;

			std::atomic<bool> m_stopped;
			bool m_debugging;

			std::map<word, Debugger::Watchpoint> m_watchpoints;

			bool m_break;
			bool m_enable_watchpoints;

			bool m_enable_stacktrace;

			std::vector<Debugger::StacktraceEntry> m_stacktrace;

			
			std::chrono::steady_clock::time_point m_last_frame{};

			using replace_values = std::vector<std::pair<word, byte>>;
			using cheat_pair = std::pair<Cheats::GameGenie, replace_values>;

			std::map<std::string, cheat_pair> m_genies;
			std::map<std::string, Cheats::GameShark> m_sharks;

		public:
			/*
			* Creates the Cartridge objects, reading from
			* the file passed as argument.
			* After that, initialization of cpu and memory takes
			* place
			*
			* @param filename The rom file
			*/
			EmulatorState(std::string_view const& filename, Logger& log);

			/*
			* Synchronization of Timer, PPU and other
			* things
			*/
			void Sync(byte cycles);

			CPU::Cpu* GetCPU();
			Mem::Memory* GetMemory();
			Cartridge::MemoryCard* GetCard();
			Graphics::PPU* GetPPU();
			Timing::Timer* GetTimer();
			Input::Joypad* GetJoypad();
			Sound::APU* GetAPU();
			DataTransfer::Serial* GetSerial();

			std::string const& GetFilename() const;

			bool Ok() const;
			std::string const& GetMessage() const;

			Logger& GetLogger();

			void Stop();
			void SetStopped(bool stop);

			bool Stopped() const;

			void ShowFrame(byte* framebuffer);

			~EmulatorState();

			inline bool IsDebugging() const {
				return m_debugging;
			}

			void SetDebugging(bool val);

			std::map<word, Debugger::Watchpoint> const& GetWatchpoints() const;
			std::map<word, Debugger::Watchpoint>& GetWatchpoints();
			
			bool WatchpointsEnabled() const;
			void EnableWatchpoints(bool value);

			inline bool StacktraceEnabled() const {
				return m_enable_stacktrace;
			}

			void EnableStacktrace(bool value);

			void StacktraceClear();
			word StacktraceGetSize() const;

			std::vector<Debugger::StacktraceEntry> const& GetStacktrace() const;

			void Break(bool val);

			bool ShouldBreak() const;

			void add_stacktrace_entry(word callee, 
				word dest, word ret, byte page);
			void remove_stacktrace_entry(word ret_address);
		
			void SerialConnect(std::string const& to);
			void SerialListen(std::string const& on);
			void SerialDisconnect();

			std::string AddGenie(std::string const& cheat);
			std::string RemoveGenie(std::string const& cheat);

			std::vector<std::string> GetGenies() const;

			std::string AddShark(std::string const& cheat);
			std::string RemoveShark(std::string const& cheat);

			std::vector<std::string> GetShark() const;

		/// <summary>
		/// Options
		/// </summary>
			void UseBootrom(std::string const& path);
		
		private :
			void ApplySharks();
		};
	}
}