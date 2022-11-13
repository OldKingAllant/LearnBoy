#pragma once

#include "cli/include/cli/cli.h"
#include "cli/include/cli/loopscheduler.h"
#include "cli/include/cli/clilocalsession.h"

#include <vector>

#include "./include/common/Common.h"

namespace GameboyEmu::State {
	class EmulatorState;
}

namespace GameboyEmu::Debugger {
	class DebuggerClass;
}

class CliManager {
public :
	/*
	* Constructs a Cli Manager, 
	* using emulator components
	* found in objs
	*/
	CliManager(GameboyEmu::State::EmulatorState* objs);

	/*
	* Runs the cli loop (does not exit 
	* until the user stops emulation)
	*/
	void Run();

	GameboyEmu::Debugger::DebuggerClass* GetDebugger();

	~CliManager();

private :
	void step(std::ostream& out);
	void continue_(std::ostream& out);
	void next_(std::ostream& out);
	void brstatus(std::ostream& out);
	void brlist(std::ostream& out);

	void bradd(
		std::ostream& out, 
		byte page,
		word address,
		word hitrate
	);

	void brchange(
		std::ostream& out,
		byte page,
		word address,
		word hitrate
	);

	void brdel(
		std::ostream& out,
		byte page,
		word address
	);

	void brtoggle(
		std::ostream& out,
		byte page,
		word address,
		bool val
	);

	void disassemble_1(std::ostream& out, word address);
	void disassemble_rng(std::ostream& out, word address, word end);

	void game_save(std::ostream& out, std::string const& path);
	void load_save(std::ostream& out, std::string const& path);

	void savestate_save(std::ostream& out, std::string const& path);
	void savestate_load(std::ostream& out, std::string const& path);
	/*
	* Inits all the command handlers
	*/
	void init_commands();

private :
	//Unique pointer to the root Menu (reset right after creation)
	std::unique_ptr<cli::Menu> root;
	//Secondary pointer to main menu (never reset)
	cli::Menu* pointerToRoot;
	//Command scheduler
	cli::LoopScheduler sched;
	//Cli object
	cli::Cli cli;
	cli::CliLocalTerminalSession session;

	//The emulator state
	GameboyEmu::State::EmulatorState* state;
	GameboyEmu::Debugger::DebuggerClass* debugger;
};