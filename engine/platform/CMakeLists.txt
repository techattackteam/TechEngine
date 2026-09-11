techengine_module(platform
  SOURCES src/Platform.cpp
          src/ExecutablePath.cpp
          src/input/InputBuffer.cpp
          src/input/InputState.cpp
          src/files/FileAccess.cpp
          src/files/MountTable.cpp
          src/files/VirtualPath.cpp
          src/window/Window.cpp
  HEADERS include/TechEngine/platform/Platform.hpp
          include/TechEngine/platform/input/InputBuffer.hpp
          include/TechEngine/platform/input/InputState.hpp
          include/TechEngine/platform/files/FileAccess.hpp
          include/TechEngine/platform/files/FileResult.hpp
          include/TechEngine/platform/files/MountTable.hpp
          include/TechEngine/platform/files/VirtualPath.hpp
          include/TechEngine/platform/window/Window.hpp
  DEPS    base
  LIBS    glfw
  ) # OS seam: window/input (ADR-006 §1).

if(TE_BUILD_TESTS)
  techengine_test(platform SOURCES
          tests/ExecutablePathTests.cpp
          tests/files/FileAccessTests.cpp
          tests/files/MountTableTests.cpp
          tests/files/VirtualPathTests.cpp
          tests/input/InputBufferTests.cpp
          tests/window/WindowTests.cpp)
  target_link_libraries(TechEnginePlatformTests PRIVATE Threads::Threads)
endif()
