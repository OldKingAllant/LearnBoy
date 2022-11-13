#include "CliManager.h"

#include "include/cpu/Cpu.h"
#include "include/state/EmulatorState.h"
#include "include/graphics/ppu/PPU.h"
#include "include/cpu/Disasm.h"
#include "include/debugger/Debugger.h"

#include "include/save/GameSave.h"
#include "include/save/Savestate.h"

#include <vector>

using EmulatorState = GameboyEmu::State::EmulatorState;
using Cpu = GameboyEmu::CPU::Cpu;
using PPU = GameboyEmu::Graphics::PPU;
using Memory = GameboyEmu::Mem::Memory;
using GameboyEmu::CPU::Disassemble;

std::vector<word> parseHexadecimal(
    std::string const& from, unsigned num_params
) {
    std::stringstream stream(from);

    std::vector<word> ret{};

    ret.reserve(num_params);

    while (!stream.eof() && !stream.fail()
        && num_params > 0) {
        word in = 0;

        stream >> std::hex >> in;

        ret.push_back(in);

        num_params--;
    }

    return ret;
}

void CliManager::init_commands() {
    //
    pointerToRoot->Insert("step", std::move( [this](std::ostream& out) {
        this->step(out);
    }));

    pointerToRoot->Insert("continue", std::move( [this](std::ostream& out) {
        this->continue_(out);
    }));

    pointerToRoot->Insert("next", std::move([this](std::ostream& out) {
        this->next_(out);
    }));

    auto breakpoints = std::make_unique<cli::Menu>("br");

    breakpoints->Insert("status", std::move([this](std::ostream& out) {
        this->brstatus(out);
    }));

    breakpoints->Insert("disable", std::move([this](std::ostream& out) {
        this->debugger->DisableBreakpoints();
    }));

    breakpoints->Insert("enable", std::move([this](std::ostream& out) {
        this->debugger->EnableBreakpoints();
    }));

    breakpoints->Insert("clear", std::move([this](std::ostream& out) {
        this->debugger->ClearBreakpointList();
    }));

    breakpoints->Insert("list", std::move([this](std::ostream& out) {
        this->brlist(out);
    }));

    breakpoints->Insert("disableall", std::move([this](std::ostream& out) {
        this->debugger->BreakpointsDisableAll();
    }));

    breakpoints->Insert("enableall", std::move([this](std::ostream& out) {
        this->debugger->BreakpointsEnableAll();
    }));

    breakpoints->Insert("set", std::move([this](std::ostream& out, std::string address_str) {
        auto params = parseHexadecimal(address_str, 1);

        if (params.size() == 0) {
            out << "Invalid input on argument 0" << std::endl;
            return;
        }

        this->bradd(out, 0, params[0], 1);
    }));

    breakpoints->Insert("set", std::move([this](std::ostream& out, std::string page_str,
        std::string address_str) {
        auto params = parseHexadecimal(page_str
            + " " + address_str, 2);

        if (params.size() < 2) {
            out << "Invalid input on argument " << 
                params.size() << std::endl;
            return;
        }

        this->bradd(out, (byte)params[0], params[1], 1);
    }));

    breakpoints->Insert("set", std::move([this](std::ostream& out, std::string page_str,
        std::string address_str, std::string hitrate_str) {
            auto params = parseHexadecimal(page_str
                + " " + address_str + " " + 
                hitrate_str, 3);

            if (params.size() < 3) {
                out << "Invalid input on argument " <<
                    params.size() << std::endl;
                return;
            }

            this->bradd(out, (byte)params[0], params[1], params[2]);
        }));

    breakpoints->Insert("change", std::move([this](std::ostream& out, std::string page_str,
        std::string address_str, std::string hitrate_str) {
            auto params = parseHexadecimal(page_str
                + " " + address_str + " " +
                hitrate_str, 3);

            if (params.size() < 3) {
                out << "Invalid input on argument " <<
                    params.size() << std::endl;
                return;
            }

            this->brchange(out, (byte)params[0], params[1], params[2]);
        }));

    breakpoints->Insert("delete", std::move([this](std::ostream& out, std::string page_str,
        std::string address_str) {
            auto params = parseHexadecimal(page_str
                + " " + address_str, 2);

            if (params.size() < 2) {
                out << "Invalid input on argument " <<
                    params.size() << std::endl;
                return;
            }

            this->brdel(out, (byte)params[0], params[1]);
        }));

    breakpoints->Insert("toggle", std::move([this](std::ostream& out, std::string page_str,
        std::string address_str, bool value) {
            auto params = parseHexadecimal(page_str
                + " " + address_str, 2);

            if (params.size() < 2) {
                out << "Invalid input on argument " <<
                    params.size() << std::endl;
                return;
            }

            this->brtoggle(out, (byte)params[0], params[1], value);
        }));

    pointerToRoot->Insert(std::move(breakpoints));



    auto disassemble = std::make_unique<cli::Menu>("d");

    disassemble->Insert("here", std::move([this](std::ostream& out) {
        word ip = this->state->
            GetCPU()->GetIP();

        this->disassemble_1(out, ip);
    }));

    disassemble->Insert("at", std::move([this](std::ostream& out, 
        std::string addr_str) {
        auto params = parseHexadecimal(addr_str, 1);

        if (params.size() == 0) {
            out << "Invalid input on argument 0" 
                << std::endl;
            return;
        }

        this->disassemble_1(out, params[0]);
    }));

    disassemble->Insert("rng", std::move([this](std::ostream& out,
        std::string addr_str, std::string end_str) {
            auto params = parseHexadecimal(addr_str + 
                " " + end_str, 2);

            if (params.size() < 2) {
                out << "Invalid input on argument "
                    << params.size()
                    << std::endl;
                return;
            }

            this->disassemble_rng(out, params[0], params[1]);
    }));

    pointerToRoot->Insert(std::move(disassemble));

    auto stacktrace = std::make_unique<cli::Menu>("stacktrace");

    stacktrace->Insert("enable", [this](std::ostream& out) {
        this->debugger->EnableStacktrace();
    });

    stacktrace->Insert("disable", [this](std::ostream& out) {
        this->debugger->DisableStacktrace();
    });

    stacktrace->Insert("clear", [this](std::ostream& out) {
        this->debugger->StacktraceClear();
    });

    stacktrace->Insert("status", [this](std::ostream& out) {
        out << "Stacktrace enabled : "
            << this->debugger->StacktraceEnabled()
            << "\n";

        out << "Stacktrace size : "
            << this->debugger->StacktraceGetSize()
            << "\n";
    });

    pointerToRoot->Insert(std::move(stacktrace));

    pointerToRoot->Insert("backtrace", [this](std::ostream& out) {
        auto backtrace = this->debugger->Backtrace();

        while (!backtrace.empty()) {
            out << std::format("0x{:x} ?? ()\n",
                backtrace.top().callee);

            backtrace.pop();
        }
    });

    pointerToRoot->Insert("stepout", [this](std::ostream& out) {
        this->debugger->StepOut(out);
    });

    auto save = std::make_unique<cli::Menu>("game");

    save->Insert("save", [this](std::ostream& out, std::string path) {
        this->game_save(out, path);
    });

    save->Insert("load", [this](std::ostream& out, std::string path) {
        this->load_save(out, path);
    });

    pointerToRoot->Insert(std::move(save));

    auto serial = std::make_unique<cli::Menu>("serial");

    serial->Insert("listen", [this](std::ostream& out, std::string on) {
        this->state->SerialListen(on);
    });

    serial->Insert("connect", [this](std::ostream& out, std::string to) {
        this->state->SerialConnect(to);
    });

    serial->Insert("disconnect", [this](std::ostream& out) {
        this->state->SerialDisconnect();
    });

    pointerToRoot->Insert(std::move(serial));

    auto savestate = std::make_unique<cli::Menu>("state");

    savestate->Insert("save", [this](std::ostream& out, std::string to) {
        this->savestate_save(out, to);
    });

    savestate->Insert("load", [this](std::ostream& out, std::string from) {
        this->savestate_load(out, from);
    });

    pointerToRoot->Insert(std::move(savestate));

    auto genie = std::make_unique<cli::Menu>("genie");

    genie->Insert("add", [this](std::ostream& out, std::string cheat) {
        std::string res = this->state->AddGenie(cheat);

        if (!res.empty()) {
            out << res << std::endl;
        }
    });

    genie->Insert("remove", [this](std::ostream& out, std::string cheat) {
        std::string res = this->state->RemoveGenie(cheat);
    
        if (!res.empty()) {
            out << res << std::endl;
        }
    });

    genie->Insert("list", [this](std::ostream& out) {
        auto list = this->state->GetGenies();

        for (std::string const& genie : list) {
            std::cout << genie << std::endl;
        }
    });

    genie->Insert("clear", [this](std::ostream& out) {
        auto list = this->state->GetGenies();

        for (std::string const& genie : list) {
            this->state->RemoveGenie(genie);
        }
    });

    pointerToRoot->Insert(std::move(genie));

    auto shark = std::make_unique<cli::Menu>("shark");

    shark->Insert("add", [this](std::ostream& out, std::string cheat) {
        std::string res = this->state->AddShark(cheat);

        if (!res.empty()) {
            out << res << std::endl;
        }
    });

    shark->Insert("remove", [this](std::ostream& out, std::string cheat) {
        std::string res = this->state->RemoveShark(cheat);

        if (!res.empty()) {
            out << res << std::endl;
        }
    });

    shark->Insert("list", [this](std::ostream& out) {
        auto list = this->state->GetShark();

        for (std::string const& genie : list) {
            std::cout << genie << std::endl;
        }
    });

    shark->Insert("clear", [this](std::ostream& out) {
        auto list = this->state->GetShark();

        for (std::string const& genie : list) {
            this->state->RemoveShark(genie);
        }
    });

    pointerToRoot->Insert(std::move(shark));

    pointerToRoot->Insert("detach", [this](std::ostream& out) {
        this->debugger->Detach();
    });

    pointerToRoot->Insert("attach", [this](std::ostream& out) {
        this->debugger->Attach();
    });
}

CliManager::CliManager(EmulatorState* state) 
	: root(std::make_unique<cli::Menu>("emu")),
      pointerToRoot(root.get()),
      sched(), cli(std::move(root)),
      session(cli, sched, std::cout), 
      state(state), debugger(nullptr) {

	session.ExitAction([this](auto& out) {
		sched.Stop();

        this->state->Stop();
	});

    debugger = new GameboyEmu::Debugger::DebuggerClass();

    debugger->SetState(state);

    init_commands();
}

void CliManager::Run() {
	sched.Run();
}

CliManager::~CliManager() {}

void OutputCpuState(std::ostream& out,
    Cpu* cpu) {
    auto const& ctx = cpu->GetContext();

    out << std::format(
        "IP = 0x{0:x} "
        "SP = 0x{1:x} "
        "AF = 0x{2:x} "
        "BC = 0x{3:x} "
        "DE = 0x{4:x} "
        "HL = 0x{5:x} "
        "IME = {6}",
        ctx.ip, ctx.sp, ctx.af, ctx.bc,
        ctx.de, ctx.hl, ctx.enableInt
    );
}

void CliManager::step(std::ostream& out) {
    if (state->Stopped()) {
        sched.Stop();
        return;
    }

    if (!debugger->IsDebugging()) {
        out << "Not in debug mode\n";
        return;
    }

    word currentip = state->GetCPU()->GetIP();

    auto res = Disassemble(currentip, state->GetMemory());

    out << std::format(
        "{0} at 0x{1:x}\n",
        res.first,
        currentip
    );

    out << "Before ";

    Cpu* cpu = state->GetCPU();

    OutputCpuState(out, cpu);

    out << "\n";

    debugger->Step(out, true);

    if (cpu->GetContext().halted) {
        out << "HALTED\n";
        return;
    }

    out << "After ";

    OutputCpuState(out, cpu);

    out << "\n";
}

void CliManager::continue_(std::ostream& out) {
    if (state->Stopped()) {
        sched.Stop();
        return;
    }

    if (!debugger->IsDebugging()) {
        out << "Not in debug mode\n";
        return;
    }

    debugger->Continue(out, true);
}

void CliManager::brstatus(std::ostream& out) {
    out << "Enabled/disabled : " << debugger->BreakpointsEnabled() << "\n";

    out << "Number of breakpoints : " << debugger->GetBreakpoints()
        .size() << "\n";
}

void CliManager::brlist(std::ostream& out) {
    auto const& breakpoints = debugger->GetBreakpoints();

    out << "Number of breakpoints : " << breakpoints
        .size() << "\n";

    for (auto const& key_value : breakpoints) {
        word address = key_value.first;
        auto const& br = key_value.second;

        out << std::format(
            "0x{0:x} : {{"
            " bank = 0x{1:x}"
            " hitrate = 0x{2:x}"
            " required_hitrate = 0x{3:x}"
            " enable = {4}"
            " }}"
        , address, br.bank,
            br.hitrate, br.require_hitrate,
            br.enabled) << std::endl;
    }
}

void CliManager::bradd(
    std::ostream& out,
    byte page,
    word address,
    word hitrate
) {
    this->debugger->BreakpointSet(page, address, hitrate, out, true);
}

void CliManager::brchange(
    std::ostream& out,
    byte page,
    word address,
    word hitrate
) {
    this->debugger->BreakpointChange(page, address, hitrate, out, true);
}

void CliManager::brdel(
    std::ostream& out,
    byte page,
    word address
) {
    this->debugger->BreakpointDelete(page, address, out, true);
}

void CliManager::brtoggle(
    std::ostream& out,
    byte page,
    word address,
    bool val
) {
    this->debugger->BreakpointToggle(page, address, val, out, true);
}

void CliManager::next_(std::ostream& out) {
    this->debugger->Next(out, true);
}

void CliManager::disassemble_1(std::ostream& out, word address) {
    auto dis = GameboyEmu::CPU::Disassemble(address,
        state->GetMemory());

    out << std::format("0x{0:x} | {1}\n",
        address, dis.first);
}

void CliManager::disassemble_rng(std::ostream& out, word address, word end) {
    if (address >= end) {
        out << "Invalid range\n";
        return;
    }

    auto mem = state->GetMemory();

    while (address < end) {
        auto dis = GameboyEmu::CPU::Disassemble(
            address, mem
        );

        out << std::format("0x{0:x} | {1}\n",
            address, dis.first);

        address += dis.second;
    }
}

void CliManager::game_save(std::ostream& out, std::string const& path) {
    auto res = GameboyEmu::Saves::SaveGame(
        state->GetCard(), path
    );

    if (!res.first) {
        out << res.second << std::endl;
    }
}

void CliManager::load_save(std::ostream& out, std::string const& path) {
    auto res = GameboyEmu::Saves::LoadGame(
        state->GetCard(), path
    );

    if (!res.first) {
        out << res.second << std::endl;
    }
}

void CliManager::savestate_save(std::ostream& out, std::string const& path) {
    auto res = GameboyEmu::Saves::SaveState(
        path, state
    );

    if (!res.first) {
        out << res.second << std::endl;
    }
}

void CliManager::savestate_load(std::ostream& out, std::string const& path) {
    auto res = GameboyEmu::Saves::LoadState(
        path, state
    );

    if (!res.first) {
        out << res.second << std::endl;
    }
}

GameboyEmu::Debugger::DebuggerClass* CliManager::GetDebugger() {
    return debugger;
}