DIR="$(pwd)"
mkdir 3rdparty
mkdir 3rdparty/libs
mkdir 3rdparty/SDL2
mkdir 3rdparty/Poco
mkdir 3rdparty/fmt
cd ./Modules/SDL2
mkdir -p build && cd build
export PREFIX=$DIR/3rdparty/SDL2
echo $DIR
cmake -DCMAKE_INSTALL_PREFIX=$PREFIX .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target install --config Release
export PREFIX=$DIR/3rdparty/Poco
cd ../../Poco
mkdir -p build_poco && cd build_poco
cmake -DCMAKE_INSTALL_PREFIX=$PREFIX ..
cmake --build . --target install --config Release
cd ../../fmt
export PREFIX=$DIR/3rdparty/fmt
mkdir -p build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$PREFIX ..
cmake --build . --target install --config Release
cd $DIR
cmake .
