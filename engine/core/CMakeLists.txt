techengine_module(core
  SOURCES src/Core.cpp
          src/events/EventRegistry.cpp
          src/events/EventStream.cpp
          src/events/EventStreamManager.cpp
          src/jobs/DedicatedThread.cpp
          src/jobs/DedicatedThreadContext.cpp
          src/jobs/DedicatedThreadState.cpp
          src/jobs/JobSystem.cpp
          src/jobs/ThreadRegistration.cpp
          src/scene/Archetype.cpp
          src/scene/ArchetypeStorage.cpp
          src/scene/ComponentRegistry.cpp
          src/scene/EntitySlots.cpp
          src/scene/Scene.cpp
          src/scene/Transform.cpp
          src/serialization/Reader.cpp
          src/serialization/Writer.cpp
  HEADERS include/TechEngine/core/Core.hpp
          include/TechEngine/core/EngineContext.hpp
          include/TechEngine/core/SimulationContext.hpp
          include/TechEngine/core/TimingMetrics.hpp
          include/TechEngine/core/events/EventRegistry.hpp
          include/TechEngine/core/events/EventTypeId.hpp
          include/TechEngine/core/events/EventStream.hpp
          include/TechEngine/core/events/EventStreamManager.hpp
          include/TechEngine/core/jobs/DedicatedThread.hpp
          include/TechEngine/core/jobs/DedicatedThreadContext.hpp
          include/TechEngine/core/jobs/JobSystem.hpp
          include/TechEngine/core/jobs/ThreadRegistration.hpp
          include/TechEngine/core/scene/Access.hpp
          include/TechEngine/core/scene/ComponentRegistry.hpp
          include/TechEngine/core/scene/ComponentStorage.hpp
          include/TechEngine/core/scene/ComponentTypeId.hpp
          include/TechEngine/core/scene/Entity.hpp
          include/TechEngine/core/scene/Query.hpp
          include/TechEngine/core/scene/Scene.hpp
          include/TechEngine/core/scene/components/Transform.hpp
          include/TechEngine/core/serialization/BlobHeader.hpp
          include/TechEngine/core/serialization/Reader.hpp
          include/TechEngine/core/serialization/Visit.hpp
          include/TechEngine/core/serialization/Writer.hpp
          src/jobs/DedicatedThreadState.hpp
          src/jobs/RegisteredThread.hpp
          src/scene/Archetype.hpp
          src/scene/ArchetypeStorage.hpp
          src/scene/EntitySlots.hpp
          src/scene/components/Hierarchy.hpp
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
          tests/jobs/DedicatedThreadTests.cpp
          tests/jobs/JobSystemTests.cpp
          tests/scene/ArchetypeStorageTests.cpp
          tests/scene/ArchetypeTests.cpp
          tests/scene/ComponentRegistryTests.cpp
          tests/scene/EntitySlotsTests.cpp
          tests/scene/HierarchyTests.cpp
          tests/scene/QueryTests.cpp
          tests/scene/TransformTests.cpp
          tests/serialization/VisitTests.cpp
          tests/serialization/WriterReaderTests.cpp)
endif()
