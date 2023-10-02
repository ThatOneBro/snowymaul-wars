cmake_minimum_required(VERSION 3.5)
project(snake LANGUAGES C)

add_subdirectory(deps/glad/)

add_executable(snake snake.c)
target_include_directories(snake PRIVATE ./deps/glad/include)
target_include_directories(snake PUBLIC /opt/homebrew/opt/glfw/include)
target_link_libraries(snake glad)
target_link_libraries(snake /opt/homebrew/opt/glfw/lib/libglfw.dylib)

if (EMSCRIPTEN)
  set(CMAKE_C_COMPILER emcc)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -sUSE_GLFW=3 -sASSERTIONS=1 -sWASM=1 -sASYNCIFY -sFULL_ES3 --ignore-dynamic-linking")
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif (EMSCRIPTEN)
