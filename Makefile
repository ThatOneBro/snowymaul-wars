
.PHONY: clean build-native-release build-native-debug build-emscripten run all-debug all-emscripten all-release

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

run-wasm:
	emrun --browser chrome ./build/snake.html

all-debug: clean build-native-debug run
all-emscripten: clean build-emscripten run-wasm
all-release: clean build-native-release run
