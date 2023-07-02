cmake_minimum_required (VERSION 3.20.0)
project (lox_vm VERSION 1.0.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")

add_executable(clox ${SOURCES})

target_include_directories(clox PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_sources(clox PUBLIC ${HEADERS})