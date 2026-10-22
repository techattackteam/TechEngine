# clientapp: the App subclass that hosts the client, shared by runtime and editor. It lives
# under apps/, not engine/, because app stays presentation-agnostic and a runtime-server must
# not link the client (ADR-006 §1).
techengine_module(clientapp
  SOURCES src/ClientApp.cpp
  HEADERS include/TechEngine/clientapp/ClientApp.hpp
  DEPS    app client)
