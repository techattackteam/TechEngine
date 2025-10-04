file(GLOB_RECURSE ENGINE_CORE_SRC "src/*.cpp")
file(GLOB_RECURSE ENGINE_CORE_PUBLIC_HEADERS "include/**/*.hpp")
file(GLOB_RECURSE ENGINE_CORE_PRIVATE_HEADERS "src/*.hpp")

find_package(GLM REQUIRED)
find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(assimp REQUIRED)
find_package(JoltPhysics REQUIRED)

option(PHYSICS_DEBUG_RENDERER "Enable the physics debug renderer" OFF)

add_library(TechEngineCore STATIC
        ${ENGINE_CORE_SRC}
        ${ENGINE_CORE_PRIVATE_HEADERS}
        ${ENGINE_CORE_PUBLIC_HEADERS}
        "${CMAKE_SOURCE_DIR}/libs/stb_image/stb_image.h"
        "src/audio/miniaudio.h"
)

target_include_directories(TechEngineCore
        PUBLIC
        include
        src
        ${CMAKE_SOURCE_DIR}/libs/stb_image
)

target_link_libraries(TechEngineCore
        PUBLIC
        glm
        spdlog
        yaml-cpp::yaml-cpp
        assimp
        Jolt
)

target_compile_definitions(TechEngineCore PUBLIC GLM_ENABLE_EXPERIMENTAL)
target_compile_definitions(TechEngineCore PRIVATE TECH_ENGINE_CORE_EXPORT)

target_compile_definitions(TechEngineCore PUBLIC
        $<$<CONFIG:DEBUG>:TECH_ENGINE_CORE_DEBUG>
        $<$<CONFIG:RELEASE>:TECH_ENGINE_CORE_RELEASE>
        $<$<CONFIG:RelWithDebInfo>:TECH_ENGINE_CORE_RELWITHDEBINFO>)

if (PHYSICS_DEBUG_RENDERER)
    target_compile_definitions(TechEngineCore PUBLIC PHYSICS_DEBUG_RENDERER)
endif ()

#Setup target's output directory
if (WIN32) # WINDOWS
    set(OUTPUT "${PROJECT_SOURCE_DIR}/bin/engine/core")

    # Set the output directory
    set_target_properties(TechEngineCore PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
    )
    set_target_properties(TechEngineCore PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
    )
    set_target_properties(TechEngineCore PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUTPUT}/relwithdebinfo"
            LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUTPUT}/relwithdebinfo"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUTPUT}/relwithdebinfo"
    )
endif ()

# Copy the include files to the output directory
add_custom_command(TARGET TechEngineCore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/engine/core/include"
        "${OUTPUT}/include"
)