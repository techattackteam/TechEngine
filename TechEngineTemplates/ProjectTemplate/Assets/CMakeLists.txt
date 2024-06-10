cmake_minimum_required(VERSION 3.24)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../Resources/Common/dependencies/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "Vcpkg target triplet")
project(UserScripts)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../Resources/Common/dependencies/GameNetworkingSockets" "${CMAKE_CURRENT_BINARY_DIR}/dependencies/GameNetworkingSockets")
add_subdirectory("Client")
add_subdirectory("Server")