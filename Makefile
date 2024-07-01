
.PHONY: clean make-dirs build-emscripten build-native-debug build-native-release run-native run-emscripten all-native-debug all-native-release all-emscripten

clean:
	rm -rf build

make-dirs:
	mkdir -p build

build-emscripten: make-dirs
	emcmake cmake -S . -B build -DPLATFORM=Web -DGRAPHICS=GRAPHICS_API_OPENGL_ES3
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

all-native-debug: clean make-dirs build-native-debug run-native
all-native-release: clean make-dirs build-native-release run-native
all-emscripten: clean make-dirs build-emscripten run-emscripten
