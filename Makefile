
.PHONY: build

build:
	clang snake.c -g -O0 -o snake -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit
