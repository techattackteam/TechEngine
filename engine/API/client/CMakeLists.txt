file(GLOB_RECURSE ENGINE_API_CLIENT_SRC "src/*.cpp")
file(GLOB_RECURSE ENGINE_API_CLIENT_HEADERS "include/*.hpp")

add_library(TechEngineAPIClient SHARED ${ENGINE_API_CLIENT_SRC} ${ENGINE_API_CLIENT_HEADERS})

target_link_libraries(TechEngineAPIClient PUBLIC TechEngineAPICommon TechEngineClient)
target_compile_definitions(TechEngineAPIClient PRIVATE TECH_ENGINE_CLIENT_API_EXPORT)

target_include_directories(TechEngineAPIClient PUBLIC "src")
target_include_directories(TechEngineAPIClient PUBLIC "include")

#Setup target's output directory
if (WIN32) # WINDOWS
    set(OUTPUT "${PROJECT_SOURCE_DIR}/bin/engine/api/client")

    # Set the output directory
    set_target_properties(TechEngineAPIClient PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/lib/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/lib/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/lib/debug"
    )
    set_target_properties(TechEngineAPIClient PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/lib/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/lib/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/lib/release"
    )

    # Set target property to store the dll's path for editor and runtime to copy
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "")
        set_target_properties(TechEngineAPIClient PROPERTIES CLIENT_API_DLL "${OUTPUT}/lib/debug/TechEngineAPIClient.dll")
    else ()
        set_target_properties(TechEngineAPIClient PROPERTIES CLIENT_API_DLL "${OUTPUT}/lib/release/TechEngineAPIClient.dll")
    endif ()
endif ()