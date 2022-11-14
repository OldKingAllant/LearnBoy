cd ./SDL2
md build 2>NUL
cd build
cmake -A x64 .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
robocopy ./include/ ../../3rdparty/SDL2/ /E /S
robocopy ./include-config-release/ ../../3rdparty/SDL2/ /E /S
robocopy ./Release/ ../../3rdparty/libs/ /E /S
cd ../../Poco
md build_poco 2>NUL
cd build_poco
cmake -DCMAKE_INSTALL_PREFIX=$PREFIX -A x64 ..
cmake --build . --target install --parallel --config Release
robocopy ./bin/release/ ../../3rdparty/libs/ /E /S
robocopy ./lib/release/ ../../3rdparty/libs/ /E /S
cd ..
robocopy ./Foundation/include/Poco/ ../3rdparty/Poco/ /E /S
robocopy ./JSON/include/Poco/ ../3rdparty/Poco/ /E /S
robocopy ./Net/include/Poco/ ../3rdparty/Poco/ /E /S
robocopy ./Util/include/Poco/ ../3rdparty/Poco/ /E /S
robocopy ./XML/include/Poco/ ../3rdparty/Poco/ /E /S
robocopy ./Data/include/Poco/ ../3rdparty/Poco/ /E /S
cd ..
cmake .