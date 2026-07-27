techengine_module(base
  SOURCES src/Base.cpp src/Log.cpp src/Clock.cpp src/Assert.cpp
  HEADERS include/TechEngine/base/Base.hpp include/TechEngine/base/Log.hpp include/TechEngine/base/Clock.hpp include/TechEngine/base/Assert.hpp include/TechEngine/base/Format.hpp
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

if(TE_BUILD_TESTS)
  techengine_test(base SOURCES
          tests/BaseTests.cpp
          tests/LogTests.cpp
          tests/ClockTests.cpp
          tests/AssertTests.cpp)
endif()
