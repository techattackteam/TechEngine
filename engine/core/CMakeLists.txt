file(GLOB_RECURSE ENGINE_CORE_SRC "src/*.cpp")
file(GLOB_RECURSE ENGINE_CORE_HEADERS "src/*.hpp")

find_package(GLM REQUIRED)
find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(assimp REQUIRED)
find_package(JoltPhysics REQUIRED)

option(PHYSICS_DEBUG_RENDERER "Enable the physics debug renderer" OFF)

add_library(TechEngineCore OBJECT ${ENGINE_CORE_SRC} ${ENGINE_CORE_HEADERS})
target_include_directories(TechEngineCore PUBLIC src)
target_link_libraries(TechEngineCore PUBLIC glm spdlog yaml-cpp::yaml-cpp assimp Jolt)

target_compile_definitions(TechEngineCore PUBLIC GLM_ENABLE_EXPERIMENTAL)
target_compile_definitions(TechEngineCore PRIVATE TECH_ENGINE_CORE_EXPORT)
target_compile_definitions(TechEngineCore PUBLIC
        $<$<CONFIG:DEBUG>:TECH_ENGINE_CORE_DEBUG>
        $<$<CONFIG:RELEASE>:TECH_ENGINE_CORE_RELEASE>
        $<$<CONFIG:RelWithDebInfo>:TECH_ENGINE_CORE_RELWITHDEBINFO>)
if (PHYSICS_DEBUG_RENDERER)
    target_compile_definitions(TechEngineCore PUBLIC PHYSICS_DEBUG_RENDERER)
endif ()