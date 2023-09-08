cmake_minimum_required(VERSION 3.24)
project(UserProject)

set(CMAKE_CXX_STANDARD 20)

add_library(UserScripts SHARED Test.cpp Test.hpp)
target_include_directories(UserScripts PUBLIC C:/dev/TechEngine/bin/TechEngineEditor/debug/TechEngineAPI)
target_link_libraries(UserScripts C:/dev/TechEngine/bin/TechEngineEditor/debug/TechEngineAPI/lib/TechEngineClient.lib)
