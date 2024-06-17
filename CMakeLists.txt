cmake_minimum_required(VERSION 3.27.1)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "Vcpkg target triplet")
project(TechEngine)

if (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:library")
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
set(BUILD_SHARED_LIBS TRUE)

add_subdirectory("dependencies/GameNetworkingSockets")

add_subdirectory(TechEngineCore)
add_subdirectory(TechEngineApp/TechEngineClient)
add_subdirectory(TechEngineApp/TechEngineServer)
add_subdirectory(TechEngineRuntime/ClientRuntime)
add_subdirectory(TechEngineRuntime/ServerRuntime)
add_subdirectory(TechEngineEditor)

add_custom_target(TechEngine)
add_custom_target(Runtimes)
add_dependencies(Runtimes TechEngineRuntime TechEngineServer)
add_dependencies(TechEngine Runtimes TechEngineEditor)