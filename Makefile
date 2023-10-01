
.PHONY: clean build run all

clean:
	rm -rf build

build:
	mkdir -p build
	cmake -B./build -S.
	cmake --build ./build

run:
	./build/snake

all: clean build run
