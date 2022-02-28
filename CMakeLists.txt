cmake_minimum_required(VERSION 3.17.5)
project(TechEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
set(BUILD_SHARED_LIBS TRUE)

add_subdirectory(engine)
add_subdirectory(game)