cmake_minimum_required(VERSION 3.5)
project(td LANGUAGES C)
set(CMAKE_C_STANDARD 99)

cmake_policy(SET CMP0054 NEW)
add_executable(${PROJECT_NAME} td.c)

if (APPLE)
  set(MACOSX_DEPLOYMENT_TARGET 10.9)
  link_libraries("-framework IOKit")
  link_libraries("-framework Cocoa")
  link_libraries("-framework OpenGL")
endif (APPLE)

set(RAYLIB_SOURCE "${PROJECT_SOURCE_DIR}/deps/raylib")

add_subdirectory(${RAYLIB_SOURCE} "${PROJECT_SOURCE_DIR}/build/raylib")
add_dependencies(${PROJECT_NAME} raylib)
target_link_libraries(${PROJECT_NAME} raylib)

if (EMSCRIPTEN)
  set(CMAKE_C_COMPILER emcc)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sUSE_GLFW=3 -sASSERTIONS=1 -sWASM=1 -sGL_ENABLE_GET_PROC_ADDRESS=1 -sTOTAL_MEMORY=67108864")
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif (EMSCRIPTEN)
