cmake_minimum_required(VERSION 3.24)

project(UserScripts)
set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cache/common/findCmake")

add_subdirectory("client")