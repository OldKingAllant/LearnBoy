CMAKE_MINIMUM_REQUIRED(VERSION 3.16)

PROJECT(LearnBoy CXX)

SET(CMAKE_CXX_STANDARD 20)

SET(SRC 
	./source/cartridge/CardUtils.cpp
	./source/cartridge/CartridgeCreator.cpp
	./source/cartridge/Mbc1.cpp
	./source/cartridge/Mbc3.cpp
	./source/cartridge/MemoryCard.cpp
	./source/cartridge/RomOnly.cpp
	./source/cpu/Cpu.cpp
	./source/cpu/CpuInstr.cpp
	./source/cpu/Disasm.cpp
	./source/datatransfer/Serial.cpp
	./source/datatransfer/SerialDevice.cpp
	./source/datatransfer/out/UdpSerial.cpp
	./source/debugger/Debugger.cpp
	./source/graphics/display/Display.cpp
	./source/graphics/ppu/PixelFifos.cpp
	./source/graphics/ppu/PixelQueue.cpp
	./source/graphics/ppu/PPU.cpp
 	./source/graphics/ppu/PPU_Modes.cpp
	./source/input/Joypad.cpp
	./source/logging/Logger.cpp
	./source/memory/Memory.cpp
	./source/save/GameSave.cpp
	./source/save/Savestate.cpp
	./source/sound/output/SdlOutput.cpp
	./source/sound/apu/APU.cpp
	./source/sound/apu/Channel.cpp
	./source/sound/apu/EnvelopeSweep.cpp
	./source/sound/apu/LenCounter.cpp
	./source/sound/apu/NoiseChannel.cpp
	./source/sound/apu/PulseChannel.cpp
	./source/sound/apu/WaveChannel.cpp
	./source/sound/apu/Sequencer.cpp
	./source/sound/apu/SoundTimer.cpp
	./source/state/EmulatorState.cpp
	./source/timing/RealTimeClock.cpp
	./source/timing/Timer.cpp
	./options/OptionsParser.cpp
	./CliManager.cpp
	./GbEmulator.cpp
)

ADD_EXECUTABLE(learnboy ${SRC})


TARGET_INCLUDE_DIRECTORIES(learnboy PUBLIC ./3rdparty/)

TARGET_LINK_LIBRARIES(learnboy PUBLIC ./3rdparty/libs/SDL2)
TARGET_LINK_LIBRARIES(learnboy PUBLIC ./3rdparty/libs/PocoFoundation)
TARGET_LINK_LIBRARIES(learnboy PUBLIC ./3rdparty/libs/PocoNet)



