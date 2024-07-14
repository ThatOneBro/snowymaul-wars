
.PHONY: clean make-dirs build-emscripten build-native-debug build-native-release run-native run-emscripten run-test-ws fast-build-native-debug all-native-debug all-native-release all-emscripten

clean:
	rm -rf client/build

make-dirs:
	mkdir -p client/build

build-emscripten: make-dirs
	emcmake cmake -S ./client -B client/build -DPLATFORM=Web -DGRAPHICS=GRAPHICS_API_OPENGL_ES3
	cmake --build client/build

build-native-debug: make-dirs
	cmake -S ./client -B client/build -DCMAKE_BUILD_TYPE=Debug
	cmake --build client/build

build-native-release: make-dirs
	cmake -S ./client -B client/build -DCMAKE_BUILD_TYPE=Release
	cmake --build client/build

run-native:
	./client/build/td

run-emscripten:
	emrun --browser chrome ./client/build/td.html

run-test-ws:
	emrun --browser chrome ./client/build/test-ws.html

fast-build-native-debug: build-native-debug run-native

all-native-debug: clean make-dirs build-native-debug run-native
all-native-release: clean make-dirs build-native-release run-native
all-emscripten: clean make-dirs build-emscripten run-emscripten
