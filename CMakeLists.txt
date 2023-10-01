cmake_minimum_required(VERSION 3.5)

project(snake LANGUAGES C)

find_package(OpenGL REQUIRED)

add_executable(snake snake.c)
target_link_libraries(snake OpenGL::GL)
target_link_libraries(snake /opt/homebrew/opt/glfw/lib/libglfw.dylib)
target_include_directories(snake PRIVATE /opt/homebrew/opt/glfw/include)
