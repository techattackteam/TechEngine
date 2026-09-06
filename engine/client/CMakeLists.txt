techengine_module(client
  SOURCES      src/Client.cpp
  HEADERS      include/TechEngine/client/Client.hpp
  DEPS         core platform base
  LIBS_PRIVATE TechEngine::miniaudio TechEngine::glad)
