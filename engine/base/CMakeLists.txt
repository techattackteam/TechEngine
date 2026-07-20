techengine_module(base
  SOURCES src/Base.cpp
  HEADERS include/TechEngine/base/Base.hpp
  LIBS    glm::glm spdlog::spdlog) # base = math + logging (ADR-006 §1/§6), PUBLIC

if(TE_BUILD_TESTS)
  techengine_test(base SOURCES tests/BaseTests.cpp)
endif()
