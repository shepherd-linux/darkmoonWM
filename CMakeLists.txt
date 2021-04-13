cmake_minimum_required(VERSION 3.20)

project(darkmoonWM)

include_directories(${PROJECT_SOURCE_DIR}, "/usr/include/freetype2")
add_executable(darkmoon darkmoon.cpp draw.cpp transient.cpp util.cpp)