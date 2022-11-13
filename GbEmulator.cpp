#include <iostream>

//#include <SDL2/SDL.h>

#include "include/logging/Logger.h"
#include "include/state/EmulatorState.h"

#include "./options/OptionsParser.h"
#include "include/debugger/Debugger.h"

#include "CliManager.h"

//#undef main
#ifdef GetMessage
 #undef GetMessage
#endif

int main(int argc, char* argv[])
{
    Logger log;

    if (argc < 2) {
        std::cerr << "Invalid number of arguments\n";
        std::cerr << "Usage: " << argv[0] << " <ROM PATH> [OPTIONS]" << std::endl;
        std::cin.get();

        std::exit(0);
    }

    std::string rom_path(argv[1]);

    auto const& options = ParseOptions(argv, argc);

    GameboyEmu::State::EmulatorState emulator(rom_path, log);

    if (!emulator.Ok()) {
        std::cout << emulator.GetMessage() << std::endl;
        std::cin.get();
        std::exit(0);
    }

    CliManager cli(&emulator);

    bool use_bootrom = options.find("--enable-bootrom") != options.end()
        || options.find("--use-boot") != options.end();
    bool start_debug = options.find("--debug") != options.end()
        || options.find("--start-debug") != options.end();

    if (use_bootrom) {
        std::string bootrom_pos = "DMG_ROM.bin";

        auto boot_option = options.find("--boot");

        if (boot_option != options.end()) {
            if (boot_option->second.empty()) {
                std::cout << "--boot Requires a corresponding path" << std::endl;
                std::exit(0);
            }

            bootrom_pos = boot_option->second;
        }

        emulator.UseBootrom(bootrom_pos);
    }

    if (!start_debug) {
        cli.GetDebugger()->Detach();
    }

    cli.Run();

    std::cin.get();

    return 0;
}
