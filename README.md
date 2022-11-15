# LearnBoy
A gameboy emulator created with the intent to learn more about system emulation. 

The user interface is only command line, but future updates 
could change that. 

The system being emulated is the DMG, no Gameboy Color emulation.

<h1>Dependencies</h1>
<ul>
  <li>SDL2 For graphics and audio</li>
  <li>Poco libraries for UDP sockets</li>
  <li>Cli library</li>
</ul>

<h1>Building</h1>

The project uses c++20, which is almost completely supported by
Clang, MSVC and GCC. std::format is not supported by clang and
gcc versions under 15 and 13 respectively. It seems that for most linux
distributions this project cannot be built.

    git clone --recursive [REPOSITORY URL]
    cd [Cloned repository]
    ./build.sh (or .bat)
    cmake .

Then use make or Visual Studio to build the emulator.

<h1>Usage</h1>

From command line:

learnboy ROM PATH [OPTIONS]

The options are:
<ul>
  <li>--enable-bootrom -> Tells the emulator to use a bootrom</li>
  <li>--boot="Bootrom path" (the default is DMG_ROM.bin), must be used with --enable-bootrom</li>
  <li>--debug or --start-debug -> Starts the emulator in a paused state, for debugging</li>
</ul>

<strong>NOTICE: No ROMs or BOOTROMs are provided with this emulator, you must dump your own</strong>

The emulator provides a cli interface thanks to daniele77/cli library, providing
a debugger and a way to detach/attach to the emulation. 
The Commnand Line Interface also provides commands for:
<ul>
  <li>Inserting game genie/game shark codes</li>
  <li>Use the serial to listen on a given network port or connect the serial to a given ip:port</li>
</ul>

More updates in the future (like adding more commands and documentation)

<h1>Emulated components</h1>
<ul>
  <li>Cartridge :
    <ul>
      <li>Rom only</li>
      <li>MBC1</li>
      <li>MBC3</li>
    </ul>
  </li>
  <li>CPU (Missing instruction: STOP)</li>
  <li>PPU (Complete)</li>
  <li>Joypad</li>
  <li>Address bus</li>
  <li>APU (Complete, but some thing can be improved)</li>
  <li>Timer</li>
  <li>Serial link cable (through UDP sockets, but can be expanded)</li>
  <li>External RAM saves</li>
</ul>

<h1>Not emulated</h1>
<ul>
  <li>A bunch of cartridge types</li>
  <li>Gameboy camera/...</li>
  <li>SGB functions</li>
</ul>

<strong>You are free to download this emulator and modify it</strong>
