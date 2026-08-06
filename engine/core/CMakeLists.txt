techengine_module(core
        SOURCES src/Core.cpp src/events/EventRegistry.cpp
        HEADERS include/TechEngine/core/Core.hpp include/TechEngine/core/FrameContext.hpp include/TechEngine/core/events/EventRegistry.hpp include/TechEngine/core/events/EventTypeId.hpp
        DEPS base platform
        LIBS Jolt                             # authoritative physics (ADR-006 §1)
        LIBS_PRIVATE tomlplusplus::tomlplusplus)     # config parsing, .cpp-only
# Jolt is linked (proves it fetches + builds); its headers land with real physics
# code — the JPH_* config-define contract is settled then, not in the skeleton.
