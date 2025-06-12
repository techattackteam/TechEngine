file(GLOB_RECURSE ENGINE_API_CLIENT_SRC "src/*.cpp")
file(GLOB_RECURSE ENGINE_API_CLIENT_HEADERS "include/*.hpp")

add_library(TechEngineAPIClient SHARED ${ENGINE_API_CLIENT_SRC} ${ENGINE_API_CLIENT_HEADERS})

target_link_libraries(TechEngineAPIClient PUBLIC TechEngineAPICommon TechEngineClient)
target_compile_definitions(TechEngineAPIClient PRIVATE TECH_ENGINE_CLIENT_API_EXPORT)


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
    set_target_properties(TechEngineAPIClient PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUTPUT}/lib/relwithdebinfo"
            LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUTPUT}/lib/relwithdebinfo"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUTPUT}/lib/relwithdebinfo"
    )

    # Set target property to store the dll's path for editor and runtime to copy
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "")
        set_target_properties(TechEngineAPIClient PROPERTIES CLIENT_API_DLL "${OUTPUT}/lib/debug/TechEngineAPIClient.dll")
    elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
        set_target_properties(TechEngineAPIClient PROPERTIES CLIENT_API_DLL "${OUTPUT}/lib/relwithdebinfo/TechEngineAPIClient.dll")
    elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        set_target_properties(TechEngineAPIClient PROPERTIES CLIENT_API_DLL "${OUTPUT}/lib/release/TechEngineAPIClient.dll")
    else ()
        message(FATAL_ERROR "CMAKE_BUILD_TYPE is not set or is not recognized. Defaulting to Release.")
    endif ()
endif ()