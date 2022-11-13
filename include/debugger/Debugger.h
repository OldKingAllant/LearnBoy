#pragma once

#include "Watchpoint.h"
#include "Breakpoint.h"
#include "StacktraceEntry.h"

#include <ostream>
#include <map>
#include <optional>
#include <thread>
#include <stack>

namespace GameboyEmu::State {
	class EmulatorState;
}

/*bool operator==(br_id_t const& id1, br_id_t const& id2) {
	return id1.first == id2.first
		&& id2.second == id2.second;
}*/

namespace GameboyEmu::Debugger {
	class DebuggerClass {
	public :
		DebuggerClass();

		void SetState(State::EmulatorState* state);
		State::EmulatorState* GetState();

		void DisableBreakpoints();
		void EnableBreakpoints();

		void DisableWatchpoints();
		void EnableWatchpoints();

		bool WatchpointsEnabled() const;
		bool BreakpointsEnabled() const;

		void OutputCpuState(std::ostream& out);

		void Step(std::ostream& out, bool useout);
		void Next(std::ostream& out, bool useout);

		void Continue(std::ostream& out, bool useout);

		void BreakpointSet(byte page, word address, word hitrate, std::ostream& out, bool useout);
		void BreakpointChange(byte page, word address, word new_hitrate, std::ostream& out, bool useout);
		void BreakpointDelete(byte page, word address, std::ostream& out, bool useout);
		void BreakpointToggle(byte page, word address, bool enable, std::ostream& out, bool useout);

		void BreakpointsDisableAll();
		void BreakpointsEnableAll();

		void WatchpointsDisableAll();
		void WatchpointsEnableAll();

		void WatchpointSet(word address, byte type, Callbak&& callback, std::ostream& out, bool useout);
		void WatchpointChange(word address, byte type, std::ostream& out, bool useout);
		void WatchpointDelete(word address, std::ostream& out, bool useout);
		void WatchpointToggle(word address, bool enable, std::ostream& out, bool useout);

		std::multimap<word, Breakpoint> const& GetBreakpoints() const;
		std::map<word, Watchpoint> const& GetWatchpoints() const;

		void Attach();
		void Detach();

		bool IsDebugging() const;

		void ClearBreakpointList();
		void ClearWatchpointList();

		bool StacktraceEnabled() const;
		void EnableStacktrace();
		void DisableStacktrace();

		void StacktraceClear();
		word StacktraceGetSize() const;

		std::stack<StacktraceEntry> Backtrace();

		void StepOut(std::ostream& out);

	private :
		bool check_breakpoint(bool existsok, byte page, word address, std::ostream& out, bool useout);
		bool breakpoint_triggered(word address);

	private :
		State::EmulatorState* m_state;

		bool m_debugging;

		bool m_enable_breakpoints;
		
		std::multimap<word, Breakpoint> m_breakpoints;

		std::thread m_emu_thread;
	};
}