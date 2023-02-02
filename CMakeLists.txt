cmake_minimum_required(VERSION 3.21)
project(TechEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_WIN32_WINNT=0x0601 -DBOOST_UUID_FORCE_AUTO_LINK")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D_WIN32_WINNT=0x0601 -DBOOST_UUID_FORCE_AUTO_LINK")

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
