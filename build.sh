DIR="$(pwd)"
mkdir 3rdparty
mkdir 3rdparty/libs
mkdir 3rdparty/SDL2
mkdir 3rdparty/Poco
cd ./SDL2
mkdir -p build && cd build
export PREFIX=$DIR/3rdparty/SDL2
echo $DIR
cmake -A x64 .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
cmake --config Release --install=$PREFIX .
cp ./include/* $DIR/3rdparty/SDL2/
cp ./include-config-release/* $DIR/3rdparty/SDL2/
cp ./Release/* $DIR/3rdparty/libs/
cd ../../Poco
mkdir -p build_poco && cd build_poco
cmake -DCMAKE_INSTALL_PREFIX=$PREFIX -A x64 ..
cmake --build . --target install --parallel --config Release
cp ./bin/release/*.dll $DIR/3rdparty/libs/
cp ./lib/release/*.lib $DIR/3rdparty/libs/
cd ..
cp -r ./Foundation/include/Poco/* $DIR/3rdparty/Poco/
cp -r ./JSON/include/Poco/* $DIR/3rdparty/Poco/
cp -r ./Net/include/Poco/* $DIR/3rdparty/Poco/
cp -r ./Util/include/Poco/* $DIR/3rdparty/Poco/
cp -r ./XML/include/Poco/* $DIR/3rdparty/Poco/
cp -r ./Data/include/Poco/* $DIR/3rdparty/Poco/
cd $DIR
cmake .
