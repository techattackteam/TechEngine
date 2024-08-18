
file(GLOB_RECURSE ENGINE_API_SRC "src/*.cpp")
file(GLOB_RECURSE ENGINE_API_HEADERS "src/*.hpp")

add_library(TechEngineAPI SHARED ${ENGINE_API_SRC} ${ENGINE_API_HEADERS})

target_include_directories(TechEngineAPI PUBLIC "src")

target_link_libraries(TechEngineAPI PUBLIC TechEngineClient)

target_compile_definitions(TechEngineAPI PRIVATE TECH_ENGINE_CLIENT_API_EXPORT)
target_compile_definitions(TechEngineAPI PRIVATE GLM_ENABLE_EXPERIMENTAL)
