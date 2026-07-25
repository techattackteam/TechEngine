techengine_module(base
  SOURCES src/Base.cpp src/Log.cpp
  HEADERS include/TechEngine/base/Base.hpp include/TechEngine/base/Log.hpp
  LIBS    glm::glm spdlog::spdlog)

if(TE_BUILD_TESTS)
  techengine_test(base SOURCES
          tests/BaseTests.cpp
          tests/LogTests.cpp)
endif()
