cmake_minimum_required(VERSION 3.20)
project(darkmoonWM)


set(CMAKE_BUILD_TYPE Debug)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    file(MAKE_DIRECTORY "build/debug")
    set(PROJECT_BINARY_DIR "build/debug/")
else()
    file(MAKE_DIRECTORY "build/release")
    set(PROJECT_BINARY_DIR "build/release/")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -fpermissive -Wnarrowing")

include_directories("/usr/include/freetype2", "include")

file(GLOB SOURCES "src/*.cpp")
add_executable(darkmoon ${SOURCES})