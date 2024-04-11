cmake_minimum_required(VERSION 3.27.1)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
set(VCPKG_TARGET_TRIPLET "x64-windows")
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
project(TechEngine)

if (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:library")
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
set(BUILD_SHARED_LIBS TRUE)

# RPATH correct path for libs
set(CMAKE_INSTALL_RPATH "lib")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

add_subdirectory(TechEngineCore)
add_subdirectory(TechEngineClient)
add_subdirectory(TechEngineServer)
add_subdirectory(TechEngineEditor)
add_subdirectory(TechEngineRuntime)
