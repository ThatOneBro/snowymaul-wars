
.PHONY: clean make-dirs build-emscripten build-native-debug build-native-release run-native run-emscripten all-emscripten

clean:
	rm -rf build

make-dirs:
	mkdir -p build

build-emscripten: make-dirs
	emcmake cmake -S . -B build
	cmake --build build

build-native-debug: make-dirs
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
	cmake --build build

build-native-release: make-dirs
	cmake -S . -B build
	cmake --build build

run-native:
	./build/snake

run-emscripten:
	emrun --browser chrome ./build/snake.html

all-emscripten: clean make-dirs build-emscripten run-emscripten
