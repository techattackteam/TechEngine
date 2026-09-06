techengine_module(client
  SOURCES      src/Client.cpp
               src/render/RenderThread.cpp
  HEADERS      include/TechEngine/client/Client.hpp
               src/render/RenderThread.hpp
  DEPS         core platform base
  LIBS_PRIVATE TechEngine::miniaudio TechEngine::glad Threads::Threads)

if(TE_BUILD_TESTS)
  techengine_test(client SOURCES tests/ClientTests.cpp
                                tests/render/RenderThreadTests.cpp)
  target_link_libraries(TechEngineClientTests PRIVATE TechEngine::glad)
endif()
