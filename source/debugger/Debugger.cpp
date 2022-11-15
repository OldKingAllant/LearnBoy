#include "../../include/debugger/Debugger.h"
#include "../../include/state/EmulatorState.h"
#include "../../include/cpu/Cpu.h"
#include "../../include/cpu/Disasm.h"

namespace GameboyEmu::Debugger {
	DebuggerClass::DebuggerClass() :
		m_state(nullptr), m_debugging(true),
		m_enable_breakpoints(true),
		m_breakpoints(),
		m_emu_thread()
	{}

	void DebuggerClass::SetState(State::EmulatorState* state) {
		m_state = state;
	}

	State::EmulatorState* DebuggerClass::GetState() {
		return m_state;
	}

	void DebuggerClass::DisableBreakpoints() {
		m_enable_breakpoints = false;
	}

	void DebuggerClass::EnableBreakpoints() {
		m_enable_breakpoints = true;
	}

	void DebuggerClass::DisableWatchpoints() {
		m_state->EnableWatchpoints(false);
	}

	void DebuggerClass::EnableWatchpoints() {
		m_state->EnableWatchpoints(true);
	}

	bool DebuggerClass::WatchpointsEnabled() const {
		return m_state->WatchpointsEnabled();
	}

	bool DebuggerClass::BreakpointsEnabled() const {
		return m_enable_breakpoints;
	}

	void DebuggerClass::OutputCpuState(std::ostream& out) {
		CPU::CpuContext const& ctx = m_state->GetCPU()
			->GetContext();

		out << fmt::format(
			"Instruction Pointer : 0x{0:x}\n"
			"A : 0x{1:x}, F : {2:b}\n"
			"B : 0x{3:x}, C : 0x{4:x}\n"
			"D : 0x{5:x}, E : 0x{6:x}\n"
			"H : 0x{7:x}, L : 0x{8:x}\n"
			"SP : 0x{9:x}\n"
			"IE : {10}\n"
			"Halted : {11}\n"
			"Halt bug : {12}\n",
			ctx.ip,
			GET_HIGH(ctx.af),
			GET_LOW(ctx.af),
			GET_HIGH(ctx.bc),
			GET_LOW(ctx.bc),
			GET_HIGH(ctx.de),
			GET_LOW(ctx.de),
			GET_HIGH(ctx.hl),
			GET_LOW(ctx.hl),
			ctx.sp,
			ctx.enableInt,
			ctx.halted, ctx.haltBug
		);

		out << "\n";
	}

	void DebuggerClass::Step(std::ostream& out, bool useout) {
		m_state->GetCPU()->Step();

		if (m_state->ShouldBreak()) {
			m_state->Break(false);
		}

		m_state->SetStopped(false);
	}

	void DebuggerClass::Next(std::ostream& out, bool useout) {
		auto cpu = m_state->GetCPU();
		word ip = cpu->GetIP();

		auto mem = m_state->GetMemory();

		byte instruction = mem->Read(ip);

		static constexpr byte calls[] = {
			0xC4, 0xD4, 0xCC, 0xCD, 0xDC,
			0xC7, 0xD7, 0xE7, 0xF7, 0xCF, 
			0xDF, 0xEF, 0xFF,
		};

		if (
			std::find(
				std::begin(calls),
				std::end(calls),
				instruction
			) != std::end(calls)) {
			auto disassemble = CPU::Disassemble(
				ip, mem
			);

			word nextip = ip + disassemble.second;

			do {
				cpu->Step();
			} while (!m_state->Stopped()
				&& cpu->GetIP() != nextip);
		}
		else {
			cpu->Step();
		}

		m_state->Break(false);
		m_state->SetStopped(false);
	}

	template <typename Itr>
	Itr findBreakpoint(byte page, Itr itr, Itr end) {
		bool found = false;

		while (itr != end && !found)
		{
			found = itr->second.bank
				== page;

			if(!found)
				itr++;
		}

		return itr;
	}

	bool DebuggerClass::breakpoint_triggered(word address) {
		if (!m_enable_breakpoints || m_breakpoints.size() == 0) {
			return false;
		}

		auto id = address;

		bool ret = false;

		if (m_breakpoints.count(
			id
		)) {
			auto br_range = m_breakpoints.equal_range(id);

			if (br_range.first == br_range.second)
				return false;

			byte page = 0;

			if (address <= 0x7FFF) {
				page = m_state->GetCard()
					->GetCurrentBank(address);
			}

			decltype(br_range.first) br = 
				findBreakpoint(page, br_range.first,
					br_range.second);

			if (br == br_range.second)
				return false;

			if (br->second.enabled) {
				br->second.hitrate++;

				if (br->second.hitrate >= br->second.require_hitrate) {
					br->second.hitrate = 0;
					ret = true;
				}
			}
		}

		return ret;
	}

	void DebuggerClass::Continue(std::ostream& out, bool useout) {
		auto cpu = m_state->GetCPU();

		bool br = false;

		do {
			cpu->Step();

			br = breakpoint_triggered(cpu->GetIP());
		} while (!m_state->Stopped() 
			&& !m_state->ShouldBreak()
			&& !br);

		if (useout && br) {
			out << fmt::format("Breakpoint hit at IP = 0x{:x}\n",
				cpu->GetIP());
		}

		if (useout && m_state->ShouldBreak()) {
			out << fmt::format("Break request at IP = 0x{:x}\n",
				cpu->GetIP());
		}

		m_state->Break(false);
		m_state->SetStopped(false);
	}

	bool DebuggerClass::check_breakpoint(bool existsok, byte page, word address, std::ostream& out, bool useout) {
		auto& breaks = m_breakpoints;

		if (page != 0 && address > 0x7FFF) {
			if (useout) {
				out << fmt::format("{0}:{1:x} Is invalid\n", page,
					address);
			}

			return false;
		}


		bool found = false;

		auto range = breaks.equal_range(address);

		auto itr = findBreakpoint(page, range.first, range.second);

		found = itr != range.second;

		if (found && !existsok) {
			if (useout) {
				out << "Breakpoint already present\n";
				return false;
			}
		}
		else if (!found && existsok) {
			if (useout) {
				out << "Breakpoint not present\n";
				return false;
			}
		}

		return true;
	}

	void DebuggerClass::BreakpointSet(byte page, word address, word hitrate, std::ostream& out, bool useout) {
		auto& breaks = m_breakpoints;

		if (!check_breakpoint(false, page, address,
			out, useout)) {
			return;
		}

		Breakpoint br = {};

		br.bank = page;
		br.address = address;
		br.hitrate = 0;
		br.require_hitrate = hitrate;
		br.enabled = true;

		breaks.insert(std::pair(address, br));
	}

	void DebuggerClass::BreakpointChange(byte page, word address, word new_hitrate, std::ostream& out, bool useout) {
		auto& breaks = m_breakpoints;

		if (!check_breakpoint(true, page, address,
			out, useout)) {
			return;
		}

		auto range = breaks.equal_range(address);

		auto itr = findBreakpoint(page, range.first, range.second);

		itr->second.require_hitrate = new_hitrate;
	}

	void DebuggerClass::BreakpointDelete(byte page, word address, std::ostream& out, bool useout) {
		auto& breaks = m_breakpoints;

		if (!check_breakpoint(true, page, address,
			out, useout)) {
			return;
		}

		auto range = breaks.equal_range(address);

		auto itr = findBreakpoint(page, range.first, range.second);

		breaks.erase(itr);
	}

	void DebuggerClass::BreakpointToggle(byte page, word address, bool enable, std::ostream& out, bool useout) {
		auto& breaks = m_breakpoints;

		if (!check_breakpoint(true, page, address,
			out, useout)) {
			return;
		}

		auto range = breaks.equal_range(address);

		auto itr = findBreakpoint(page, range.first, range.second);

		itr->second.enabled = enable;
	}

	void DebuggerClass::BreakpointsDisableAll() {
		for (auto& key_val : m_breakpoints) {
			key_val.second.enabled = false;
		}
	}

	void DebuggerClass::BreakpointsEnableAll() {
		for (auto& key_val : m_breakpoints) {
			key_val.second.enabled = true;
		}
	}

	void DebuggerClass::WatchpointsDisableAll() {
		for (auto& key_val : m_state->GetWatchpoints()) {
			key_val.second.enabled = false;
		}
	}

	void DebuggerClass::WatchpointsEnableAll() {
		for (auto& key_val : m_state->GetWatchpoints()) {
			key_val.second.enabled = true;
		}
	}

	void DebuggerClass::WatchpointSet(word address, byte type, Callbak&& callback, std::ostream& out, bool useout) {
		//
	}

	void DebuggerClass::WatchpointChange(word address, byte type, std::ostream& out, bool useout) {
		//
	}

	void DebuggerClass::WatchpointDelete(word address, std::ostream& out, bool useout) {
		//
	}

	void DebuggerClass::WatchpointToggle(word address, bool enable, std::ostream& out, bool useout) {
		//
	}

	std::multimap<word, Breakpoint> const& DebuggerClass::GetBreakpoints() const {
		return m_breakpoints;
	}

	std::map<word, Watchpoint> const& DebuggerClass::GetWatchpoints() const {
		return m_state->GetWatchpoints();
	}

	void DebuggerClass::Attach() {
		if (m_debugging)
			return;

		m_state->SetStopped(true);

		m_emu_thread.join();

		m_state->SetStopped(false);

		m_debugging = true;

		m_state->SetDebugging(true);
	}

	void DebuggerClass::Detach() {
		if (!m_debugging)
			return;

		//start new emulation thread
		m_debugging = false;
		m_state->SetDebugging(false);
		m_state->SetStopped(false);

		m_emu_thread = std::thread([this]() {
			auto cpu = m_state->GetCPU();

			while (!m_state->Stopped()) {
				cpu->Step();
			}
		});
	}

	bool DebuggerClass::IsDebugging() const {
		return m_debugging;
	}

	void DebuggerClass::ClearBreakpointList() {
		decltype(m_breakpoints) clear_map;

		m_breakpoints.swap(clear_map);
	}

	void DebuggerClass::ClearWatchpointList() {
		//
	}

	bool DebuggerClass::StacktraceEnabled() const {
		return m_state->StacktraceEnabled();
	}

	void DebuggerClass::EnableStacktrace() {
		m_state->EnableStacktrace(true);
	}

	void DebuggerClass::DisableStacktrace() {
		m_state->EnableStacktrace(false);
	}

	void DebuggerClass::StacktraceClear() {
		m_state->StacktraceClear();
	}

	word DebuggerClass::StacktraceGetSize() const {
		return m_state->StacktraceGetSize();
	}

	std::stack<StacktraceEntry> DebuggerClass::Backtrace() {
		std::stack<StacktraceEntry> ret;

		auto const& stacktrace = m_state->GetStacktrace();

		for (auto it = stacktrace.crbegin(); 
			it != stacktrace.crend(); it++
			) {
			ret.push(*it);
		}

		return ret;
	}

	void DebuggerClass::StepOut(std::ostream& out) {
		auto const& stacktrace = m_state->GetStacktrace();

		if (stacktrace.empty() || 
			!m_state->StacktraceEnabled()) {
			out << "Stacktrace is empty or not enabled\n"
				<< std::endl;
			return;
		}

		auto last_entry = stacktrace.back();

		auto cpu = m_state->GetCPU();

		while (last_entry.ret_address !=
			cpu->GetIP()) {
			cpu->Step();
		}

		m_state->Break(false);
	}
}