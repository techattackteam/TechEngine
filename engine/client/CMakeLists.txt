file(GLOB_RECURSE ENGINE_CLIENT_SRC "src/*.cpp")
file(GLOB_RECURSE ENGINE_CLIENT_HEADERS "src/*.hpp")

find_package(glew REQUIRED)
find_package(glfw REQUIRED)

add_library(TechEngineClient SHARED ${ENGINE_CLIENT_SRC} ${ENGINE_CLIENT_HEADERS})

target_compile_definitions(TechEngineClient PRIVATE TECH_ENGINE_CLIENT_EXPORT TECH_ENGINE_CORE_EXPORT)

target_include_directories(TechEngineClient PUBLIC src)

target_link_libraries(TechEngineClient PUBLIC TechEngineCore glew glfw)

#Setup target's output directory
if (WIN32) # WINDOWS
    set(OUTPUT "${PROJECT_SOURCE_DIR}/bin/engine/client")

    # Set target property to store the dll's path for editor and runtime to copy
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set_target_properties(TechEngineClient PROPERTIES CLIENT_DLL "${OUTPUT}/debug/TechEngineClient.dll")
    elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
        set_target_properties(TechEngineClient PROPERTIES CLIENT_DLL "${OUTPUT}/relwithdebinfo/TechEngineClient.dll")
    elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        set_target_properties(TechEngineClient PROPERTIES CLIENT_DLL "${OUTPUT}/release/TechEngineClient.dll")
    else ()
        message(FATAL_ERROR "CMAKE_BUILD_TYPE is not set or is not recognized. Defaulting to Release.")
    endif ()

    # Set the output directory
    set_target_properties(TechEngineClient PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
    )
    set_target_properties(TechEngineClient PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
    )

    set_target_properties(TechEngineClient PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUTPUT}/relwithdebinfo"
            LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUTPUT}/relwithdebinfo"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUTPUT}/relwithdebinfo"
    )

    # Copy the glfw dll to the output directory
    add_custom_command(TARGET TechEngineClient POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${GLFW_DLL}" "${OUTPUT}/$<CONFIG>"
    )
endif ()