file(GLOB_RECURSE ENGINE_CLIENT_SRC "src/*.cpp")
file(GLOB_RECURSE ENGINE_CLIENT_HEADERS "src/*.hpp")

add_library(TechEngineClient SHARED ${ENGINE_CLIENT_SRC} ${ENGINE_CLIENT_HEADERS})

target_compile_definitions(TechEngineClient PUBLIC TECH_ENGINE_CLIENT_EXPORT)

target_include_directories(TechEngineClient PUBLIC src)

target_link_libraries(TechEngineClient PRIVATE TechEngineCore)

#Setup target's output directory
if (WIN32) # WINDOWS
    set(OUTPUT "${PROJECT_SOURCE_DIR}/bin/engine/client")

    # Set target property to store the dll's path for editor and runtime to copy
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "")
        set_target_properties(TechEngineClient PROPERTIES CLIENT_DLL "${OUTPUT}/debug/TechEngineClient.dll")
    else ()
        set_target_properties(TechEngineClient PROPERTIES CLIENT_DLL "${OUTPUT}/release/TechEngineClient.dll")
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
endif ()