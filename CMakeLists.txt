cmake_minimum_required(VERSION 3.31.6)

project(TechEngine)

set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/findCmake")

add_subdirectory(engine)
add_subdirectory(runtime)