file(GLOB_RECURSE ENGINE_API_SRC "src/*.cpp")
file(GLOB_RECURSE ENGINE_API_HEADERS "include/*.hpp")

add_library(TechEngineAPICommon OBJECT ${ENGINE_API_SRC} ${ENGINE_API_HEADERS})

target_include_directories(TechEngineAPICommon PUBLIC "src")
target_include_directories(TechEngineAPICommon PUBLIC "include")

target_link_libraries(TechEngineAPICommon PUBLIC TechEngineCore)

target_compile_definitions(TechEngineAPICommon PUBLIC TECH_ENGINE_API_EXPORT)
target_compile_definitions(TechEngineAPICommon PRIVATE GLM_ENABLE_EXPERIMENTAL)

