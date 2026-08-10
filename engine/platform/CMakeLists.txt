techengine_module(platform
  SOURCES src/Platform.cpp
          src/files/FileAccess.cpp
          src/files/MountTable.cpp
          src/files/VirtualPath.cpp
  HEADERS include/TechEngine/platform/Platform.hpp
          include/TechEngine/platform/files/FileAccess.hpp
          include/TechEngine/platform/files/FileResult.hpp
          include/TechEngine/platform/files/MountTable.hpp
          include/TechEngine/platform/files/VirtualPath.hpp
  DEPS    base
  LIBS    glfw) # OS seam: window/input (ADR-006 §1). glad2 wired once vendored.

if(TE_BUILD_TESTS)
  techengine_test(platform SOURCES
          tests/files/FileAccessTests.cpp
          tests/files/MountTableTests.cpp
          tests/files/VirtualPathTests.cpp)
endif()
