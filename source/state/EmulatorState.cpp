#include "../../include/state/EmulatorState.h"
#include "../../include/cartridge/CartridgeCreator.h"
#include "../../include/cartridge/MemoryCard.h"
#include "../../include/memory/Memory.h"
#include "../../include/cpu/Cpu.h"
#include "../../include/graphics/ppu/PPU.h"
#include "../../include/graphics/display/Display.h"
#include "../../include/timing/Timer.h"
#include "../../include/input/Joypad.h"
#include "../../include/sound/apu/APU.h"
#include "../../include/sound/output/OutputDevice.h"
#include "../../include/sound/output/SdlOutput.h"
#include "../../include/datatransfer/Serial.h"
#include "../../include/datatransfer/out/UdpSerial.h"

namespace GameboyEmu {
	namespace State {

		EmulatorState::EmulatorState(
			std::string_view const& filename, Logger& log) :
			m_file(filename), m_logger(log), m_cpu(nullptr),
			m_memory(nullptr), m_card(nullptr), m_ppu(nullptr), m_timer(nullptr),
			m_joypad(nullptr), m_apu(nullptr), m_serial(nullptr),
			m_fatal(false),
			m_fatal_message(), m_display(nullptr), m_stop_check_cycles(0),
			m_stopped(false), m_debugging(true), m_watchpoints(), m_break(false),
			m_enable_watchpoints(true), m_enable_stacktrace(false),
			m_stacktrace(), m_genies(), m_sharks() {
			m_logger.log_info("Trying to read from rom file {0}\n", m_file);
			//try to read file and create cartridge
			auto cart_or_error = Cartridge::CreateCartridge(m_file, this);

			//if the error string is not empty
			if (cart_or_error.second != "") {
				m_fatal = true;
				m_fatal_message = "Could not create cartridge, error : "
					+ cart_or_error.second;
				return;
			}

			m_logger.log_info("Cartridge created\nCartridge header : \n\n");

			m_logger.log_info("{}\n\n", cart_or_error.first->Dump());

			Sound::SdlOutputDevice* out_dev = new Sound::SdlOutputDevice(m_logger);

			m_serial = new DataTransfer::Serial(new DataTransfer::UdpSerial());
			m_apu = new Sound::APU(this, out_dev);
			m_ppu = new Graphics::PPU(this);
			m_timer = new Timing::Timer();
			m_joypad = new Input::Joypad();
			m_memory = new Mem::Memory(this, cart_or_error.first, m_ppu, m_timer, m_joypad, m_apu, m_serial);
			m_cpu = new CPU::Cpu(this, m_memory);
			m_card = cart_or_error.first;

			m_display = new Graphics::Display(m_logger, [this]() {
				this->SetStopped(true);
			});

			m_ppu->SetMemory(m_memory);
			m_timer->SetMemory(m_memory);
			m_joypad->SetMemory(m_memory);
			m_serial->SetMemory(m_memory);

			m_display->Init(160, 144, 3);

			m_stacktrace.reserve(500);

			m_display->SetJoypad(m_joypad);

			out_dev->Init();

			m_last_frame = std::chrono::steady_clock::now();
		}

		bool EmulatorState::Stopped() const {
			return m_stopped;
		}

		void EmulatorState::ShowFrame(byte* framebuffer) {
			static constexpr uint16_t fps = 60;
			static constexpr double frame_time =
				((double)1 / fps) * 1000;
			
			if (m_stopped)
				return;

			if (m_display->IsStop()) {
				m_stopped = true;
				return;
			}

			m_display->SetFrame(framebuffer);
			m_display->FramePresent();

			auto now = std::chrono::steady_clock::now();

			long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_frame).count();

			if (elapsed < frame_time) {
				auto millis_dur = 
				std::chrono::duration<double, std::milli>( frame_time - elapsed - 1 );

				auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(millis_dur);

				std::this_thread::sleep_for(millis);
			}

			ApplySharks();

			m_last_frame = std::chrono::steady_clock::now();
		}

		void EmulatorState::Sync(byte cycles) {
			m_stop_check_cycles++;

			if (m_stop_check_cycles == 500) {
				if (m_display->IsStop()) {
					m_stopped = true;
					return;
				}

				m_stop_check_cycles = 0;
			}

			m_memory->DmaAdvance(cycles);
			m_ppu->Tick(cycles);
			m_timer->Tick(cycles);
			m_apu->Tick(cycles);
			m_serial->Clock(cycles);
		}

		CPU::Cpu* EmulatorState::GetCPU() {
			return m_cpu;
		}

		Mem::Memory* EmulatorState::GetMemory() {
			return m_memory;
		}

		Cartridge::MemoryCard* EmulatorState::GetCard() {
			return m_card;
		}

		Graphics::PPU* EmulatorState::GetPPU() {
			return m_ppu;
		}

		std::string const& EmulatorState::GetFilename() const {
			return m_file;
		}

		bool EmulatorState::Ok() const {
			return !m_fatal;
		}

#undef GetMessage

		std::string const& EmulatorState::GetMessage() const {
			return m_fatal_message;
		}

		void EmulatorState::Stop() {
			m_display->Stop();

			m_stopped = true;
		}

		void EmulatorState::SetStopped(bool stop) {
			m_stopped.store(stop);
		}

		EmulatorState::~EmulatorState() {
			m_logger.log_info("Stopping emulation and destroying objects\n");

			delete m_cpu;
			delete m_memory;
			delete m_card;
			delete m_ppu;
			delete m_display;
			delete m_timer;
			delete m_joypad;
			delete m_apu;
			delete m_serial;
		}

		Logger& EmulatorState::GetLogger() {
			return m_logger;
		}

		Timing::Timer* EmulatorState::GetTimer() {
			return m_timer;
		}

		void EmulatorState::SetDebugging(bool val) {
			m_debugging = val;
		}

		std::map<word, Debugger::Watchpoint> const& EmulatorState::GetWatchpoints() const {
			return m_watchpoints;
		}

		std::map<word, Debugger::Watchpoint>& EmulatorState::GetWatchpoints() {
			return m_watchpoints;
		}

		void EmulatorState::Break(bool val) {
			m_break = val;
		}

		bool EmulatorState::ShouldBreak() const {
			return m_break;
		}

		bool EmulatorState::WatchpointsEnabled() const {
			return m_enable_watchpoints;
		}

		void EmulatorState::EnableWatchpoints(bool value) {
			m_enable_watchpoints = value;
		}

		/*bool EmulatorState::StacktraceEnabled() const {
			return m_enable_stacktrace;
		}*/

		void EmulatorState::EnableStacktrace(bool value) {
			m_enable_stacktrace = value;

			if (!m_enable_stacktrace) {
				StacktraceClear();
			}
		}

		void EmulatorState::StacktraceClear() {
			if (m_stacktrace.size() == 0)
				return;

			decltype(m_stacktrace) new_stack;

			m_stacktrace.swap(new_stack);

			m_stacktrace.reserve(500);
		}

		word EmulatorState::StacktraceGetSize() const {
			return (word)m_stacktrace.size();
		}

		std::vector<Debugger::StacktraceEntry> const& EmulatorState::GetStacktrace() const {
			return m_stacktrace;
		}

		void EmulatorState::add_stacktrace_entry(word callee,
			word dest, word ret, byte page) {
			Debugger::StacktraceEntry entry{};

			entry.callee = callee;
			entry.dest_address = dest;
			entry.ret_address = ret;
			entry.page = page;

			m_stacktrace.push_back(
				entry
			);
		}

		void EmulatorState::remove_stacktrace_entry(word ret_address) {
			if (m_stacktrace.empty())
				return;

			auto const& entry = m_stacktrace.back();

			if (entry.ret_address == ret_address) {
				m_stacktrace.pop_back();
			}
		}

		Input::Joypad* EmulatorState::GetJoypad() {
			return m_joypad;
		}

		Sound::APU* EmulatorState::GetAPU() {
			return m_apu;
		}

		DataTransfer::Serial* EmulatorState::GetSerial() {
			return m_serial;
		}

		void EmulatorState::SerialConnect(std::string const& to) {
			m_serial->Connect(to);
		}

		void EmulatorState::SerialListen(std::string const& on) {
			m_serial->Listen(on);
		}

		void EmulatorState::SerialDisconnect() {
			m_serial->CloseConnection();
		}

		std::string EmulatorState::AddGenie(std::string const& cheat) {
			Cheats::GameGenie genie;

			if (m_genies.find(cheat) != m_genies.end()) {
				return "Cheat already inserted";
			}

			if (cheat.find_first_of('-')
				== std::string::npos) {
				return "Invalid cheat";
			}

			if (cheat.size() != 11
				&& cheat.size() != 7) {
				return "Invalid cheat";
			}

			std::string newcheat = cheat;

			newcheat.erase(std::remove(newcheat.begin(),
				newcheat.end(), '-'), newcheat.end());

			std::string value = newcheat.substr(0, 2);

			byte repl_value = 0;

			bool ok = true;

			std::string address = newcheat.substr(2, 4);

			word addr_value = 0;

			try {
				repl_value = std::stoi(value, nullptr, 16);
				addr_value = std::stoi(address, nullptr, 16);
			}
			catch (std::exception const&) {
				ok = false;
			}

			if (!ok) {
				return "Invalid cheat";
			}

			std::string compare = "";

			std::optional<byte> comp_value = std::nullopt;

			if (cheat.size() == 11) {
				compare = newcheat.substr(6, 1) + 
					newcheat.substr(8, 1);

				std::string checksum = newcheat.substr(7, 1);

				byte val = 0;
				byte checksum_val = 0;

				try {
					val = std::stoi(compare, nullptr, 16);
					checksum_val = std::stoi(checksum, nullptr, 16);
				}
				catch (std::exception const&) {
					ok = false;
				}

				if (!ok) {
					return "Invalid cheat";
				}

				checksum_val ^= (val >> 4) & 0xF;

				val ^= 0xFF;
				byte bit0 = (val & 0b1);
				byte bit1 = (val & 0b10) >> 1;
				val >>= 2;
				val |= (bit0 << 6);
				val |= (bit1 << 7);
				val ^= 0x45;

				if (checksum_val >= 1 && checksum_val < 8) {
					return "Invalid cheat";
				}

				comp_value = val;
			}

			byte high_nibble = addr_value & 0xF;
			addr_value >>= 4;
			high_nibble ^= 0xF;

			addr_value |= (high_nibble << 12);

			if (addr_value > 0x7FFF) {
				return "Invalid address";
			}

			genie.cheat_string = cheat;
			genie.address = addr_value;
			genie.replace_value = repl_value;
			genie.compare_value = comp_value;

			short compare_v = comp_value.has_value() ?
				comp_value.value() : -1;

			auto list = m_card->ApplyPatch(repl_value,
				addr_value, compare_v);

			m_genies.insert(
				std::pair(cheat, cheat_pair(genie, list))
			);

			if (list.size() == 0) {
				return "No values were patched in ROM, your cheat won't produce any effect";
			}

			return "";
		}

		std::string EmulatorState::RemoveGenie(std::string const& cheat) {
			if (m_genies.find(cheat) == m_genies.end()) {
				return "Cheat not found";
			}

			auto const& genie_data = m_genies[cheat];

			auto const& genie = genie_data.first;

			m_card->RemovePatch(genie_data.second, genie.address);

			m_genies.erase(cheat);

			return "";
		}

		std::vector<std::string> EmulatorState::GetGenies() const {
			std::vector<std::string> list;

			std::transform(m_genies.begin(),
				m_genies.end(), std::back_inserter(list),
				[](auto const& pair) {
					return pair.first;
			});

			return list;
		}

		std::string EmulatorState::AddShark(std::string const& cheat) {
			if (m_sharks.find(cheat) !=
				m_sharks.end()) {
				return "Cheat already inserted";
			}

			if (cheat.size() != 8) {
				return "Invalid cheat format";
			}

			byte bank_num = 0;
			byte new_data = 0;
			word address = 0;

			bool ok = true;

			try {
				bank_num =
					std::stoi(cheat.substr(0, 2),
						nullptr, 16);

				new_data = 
					std::stoi(cheat.substr(2, 2),
						nullptr, 16);

				address =
					std::stoi(cheat.substr(4, 4),
						nullptr, 16);

				address = ((address & 0xFF) << 8) |
					((address >> 8) & 0xFF);
			}
			catch (std::exception const&) {
				ok = false;
			}

			//address += 0xA000;

			if (!ok) {
				return "Invalid cheat";
			}

			if (address < 0xA000 ||
				address > 0xDFFF) {
				return "Invalid address";
			}

			Cheats::GameShark shark{};

			shark.bank_number = bank_num;
			shark.new_data = new_data;
			shark.address = address;

			m_sharks.insert(std::pair(cheat, shark));
			
			return "";
		}

		void EmulatorState::ApplySharks() {
			for (auto& entry : m_sharks) {
				auto& shark = entry.second;

				shark.old_value = m_memory->ApplyShark(shark);
			}
		}

		std::string EmulatorState::RemoveShark(std::string const& cheat) {
			if (m_sharks.find(cheat) ==
				m_sharks.end()) {
				return "Cheat not inserted";
			}

			auto const& shark = m_sharks[cheat];

			//Remove shark from memory

			m_sharks.erase(cheat);

			return "";
		}

		std::vector<std::string> EmulatorState::GetShark() const {
			std::vector<std::string> list;

			std::transform(m_sharks.begin(),
				m_sharks.end(), std::back_inserter(list),
				[](auto const& pair) {
					return pair.first;
				});

			return list;
		}

		void EmulatorState::UseBootrom(std::string const& path) {
			m_memory->ReadBootrom(path);
			m_cpu->ResetIP();
		}
	}
}