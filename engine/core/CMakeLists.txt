
file(GLOB_RECURSE ENGINE_CORE_SRC "src/*.cpp")
file(GLOB_RECURSE ENGINE_CORE_HEADERS "src/*.hpp")

add_library(TechEngineCore OBJECT ${ENGINE_CORE_SRC} ${ENGINE_CORE_HEADERS})
target_include_directories(TechEngineCore PUBLIC src)