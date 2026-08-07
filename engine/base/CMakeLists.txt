techengine_module(base
  SOURCES src/Base.cpp
          src/diagnostics/Log.cpp
          src/diagnostics/Assert.cpp
          src/time/Clock.cpp
  HEADERS include/TechEngine/base/Base.hpp
          include/TechEngine/base/diagnostics/Log.hpp
          include/TechEngine/base/diagnostics/Assert.hpp
          include/TechEngine/base/diagnostics/FormatString.hpp
          include/TechEngine/base/diagnostics/Profile.hpp
          include/TechEngine/base/math/Math.hpp
          include/TechEngine/base/math/Format.hpp
          include/TechEngine/base/stringid/StringId.hpp
          include/TechEngine/base/stringid/Format.hpp
          include/TechEngine/base/time/Clock.hpp
  LIBS    glm::glm
  LIBS_PRIVATE spdlog::spdlog)


set(TE_LOG_ACTIVE_LEVEL "" CACHE STRING
    "Compile-time log floor (0=Trace..6=Off); empty = per-config default")

if(TE_LOG_ACTIVE_LEVEL STREQUAL "")
  set(_te_log_level "$<IF:$<CONFIG:Debug>,0,$<IF:$<CONFIG:RelWithDebInfo>,1,2>>")
else()
  set(_te_log_level "${TE_LOG_ACTIVE_LEVEL}")
endif()

target_compile_definitions(TechEngineBase PUBLIC TE_LOG_ACTIVE_LEVEL=${_te_log_level})

# RelWithDebInfo keeps TE_ASSERT on — it is the dev-runtime config (ADR-011 §5).
target_compile_definitions(TechEngineBase PUBLIC
    TE_ASSERT_DEV=$<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>,1,0>)

# PUBLIC because Profile.hpp includes <tracy/Tracy.hpp> when the option is on (ADR-013 §2).
# techengine_module()'s LIBS cannot be conditional, hence the follow-up call.
if(TE_PROFILE)
  target_link_libraries(TechEngineBase PUBLIC Tracy::TracyClient)
  target_compile_definitions(TechEngineBase PUBLIC TE_PROFILE_ENABLED)
endif()

if(TE_BUILD_TESTS)
  techengine_test(base SOURCES
          tests/BaseTests.cpp
          tests/diagnostics/LogTests.cpp
          tests/diagnostics/AssertTests.cpp
          tests/math/MathTests.cpp
          tests/math/MathFormatTests.cpp
          tests/stringid/StringIdTests.cpp
          tests/stringid/StringIdFormatTests.cpp
          tests/time/ClockTests.cpp)
endif()
