
.PHONY: clean build-native-release build-native-debug build-emscripten run all-debug all-release

clean:
	rm -rf build

build-emscripten:
	mkdir -p build
	cmake -S . -B build "-DCMAKE_TOOLCHAIN_FILE=~/repos/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
	cmake --build build

build-native-debug:
	mkdir -p build
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
	cmake --build build

build-native-release:
	mkdir -p build
	cmake -S . -B build
	cmake --build build

run:
	./build/snake

all-debug: clean build-native-debug run
all-release: clean build-native-release run
