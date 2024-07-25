cmake_minimum_required(VERSION 3.20)

project(hydrogen)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
add_executable(hydro ${SOURCE_FILES})