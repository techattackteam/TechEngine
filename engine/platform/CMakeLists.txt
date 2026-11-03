techengine_module(platform
  SOURCES src/Platform.cpp
          src/ExecutablePath.cpp
          src/input/InputBuffer.cpp
          src/input/InputState.cpp
          src/files/FileAccess.cpp
          src/files/FileError.cpp
          src/files/MountTable.cpp
          src/files/VirtualPath.cpp
          src/process/Process.cpp
          src/window/GlfwControls.cpp
          src/window/Window.cpp
          src/window/WindowError.cpp
  HEADERS include/TechEngine/platform/Platform.hpp
          include/TechEngine/platform/input/InputBuffer.hpp
          include/TechEngine/platform/input/InputState.hpp
          include/TechEngine/platform/input/Key.hpp
          include/TechEngine/platform/input/MouseButton.hpp
          include/TechEngine/platform/files/FileAccess.hpp
          include/TechEngine/platform/files/FileError.hpp
          include/TechEngine/platform/files/MountTable.hpp
          include/TechEngine/platform/files/VirtualPath.hpp
          include/TechEngine/platform/process/Process.hpp
          include/TechEngine/platform/window/Window.hpp
          include/TechEngine/platform/window/WindowError.hpp
          src/window/GlfwControls.hpp
  DEPS    base
  LIBS    glfw
  ) # OS seam: window/input (ADR-006 §1).

if(TE_BUILD_TESTS)
  techengine_test(platform SOURCES
          tests/ExecutablePathTests.cpp
          tests/files/FileAccessTests.cpp
          tests/files/FileErrorTests.cpp
          tests/files/MountTableTests.cpp
          tests/files/VirtualPathTests.cpp
          tests/input/InputBufferTests.cpp
          tests/input/InputStateTests.cpp
          tests/process/ProcessTests.cpp
          tests/window/GlfwControlsTests.cpp
          tests/window/WindowErrorTests.cpp
          tests/window/WindowTests.cpp)
  target_link_libraries(TechEnginePlatformTests PRIVATE Threads::Threads)

  add_executable(te_process_stub tests/process/ProcessStub.cpp)
  target_link_libraries(te_process_stub PRIVATE te_warnings)
  set_target_properties(te_process_stub PROPERTIES FOLDER "tests")
  add_dependencies(TechEnginePlatformTests te_process_stub)
  target_compile_definitions(TechEnginePlatformTests PRIVATE
    TE_PROCESS_STUB_PATH="$<TARGET_FILE:te_process_stub>")
endif()
