techengine_module(core
  SOURCES src/Core.cpp
          src/events/EventRegistry.cpp
          src/events/EventStream.cpp
          src/events/EventStreamManager.cpp
          src/jobs/JobSystem.cpp
          src/serialization/Reader.cpp
          src/serialization/Writer.cpp
  HEADERS include/TechEngine/core/Core.hpp
          include/TechEngine/core/EngineContext.hpp
          include/TechEngine/core/FrameContext.hpp
          include/TechEngine/core/events/EventRegistry.hpp
          include/TechEngine/core/events/EventTypeId.hpp
          include/TechEngine/core/events/EventStream.hpp
          include/TechEngine/core/events/EventStreamManager.hpp
          include/TechEngine/core/jobs/JobSystem.hpp
          include/TechEngine/core/serialization/BlobHeader.hpp
          include/TechEngine/core/serialization/Reader.hpp
          include/TechEngine/core/serialization/Writer.hpp
  DEPS    base platform
  LIBS    Jolt                             # authoritative physics (ADR-006 §1)
          Threads::Threads                 # JobSystem's workers; PUBLIC because the header has std::thread
  LIBS_PRIVATE tomlplusplus::tomlplusplus)     # config parsing, .cpp-only
# Jolt is linked (proves it fetches + builds); its headers land with real physics
# code — the JPH_* config-define contract is settled then, not in the skeleton.

if(TE_BUILD_TESTS)
  techengine_test(core SOURCES
          tests/EngineContextTests.cpp
          tests/events/EventRegistryTests.cpp
          tests/events/EventStreamTests.cpp
          tests/events/EventStreamManagerTests.cpp
          tests/jobs/JobSystemTests.cpp
          tests/serialization/WriterReaderTests.cpp)
endif()
