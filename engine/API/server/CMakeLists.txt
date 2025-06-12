file(GLOB_RECURSE ENGINE_API_SERVER_SRC "src/*.cpp")
file(GLOB_RECURSE ENGINE_API_SERVER_HEADERS "include/*.hpp")

add_library(TechEngineAPIServer SHARED ${ENGINE_API_SERVER_SRC} ${ENGINE_API_SERVER_HEADERS})

target_link_libraries(TechEngineAPIServer PUBLIC TechEngineAPICommon TechEngineServer)
#target_compile_definitions(TechEngineAPIServer PRIVATE TECH_ENGINE_CLIENT_API_EXPORT)

#Setup target's output directory
if (WIN32) # WINDOWS
    set(OUTPUT "${PROJECT_SOURCE_DIR}/bin/engine/api/server")

    # Set the output directory
    set_target_properties(TechEngineAPIServer PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/lib/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/lib/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/lib/debug"
    )
    set_target_properties(TechEngineAPIServer PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/lib/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/lib/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/lib/release"
    )
    set_target_properties(TechEngineAPIServer PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUTPUT}/lib/relwithdebinfo"
            LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUTPUT}/lib/relwithdebinfo"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUTPUT}/lib/relwithdebinfo"
    )
    # Set target property to store the dll's path for editor and runtime to copy
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set_target_properties(TechEngineAPIServer PROPERTIES SERVER_API_DLL "${OUTPUT}/lib/debug/TechEngineAPIServer.dll")
    elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
        set_target_properties(TechEngineAPIServer PROPERTIES SERVER_API_DLL "${OUTPUT}/lib/relwithdebinfo/TechEngineAPIServer.dll")
    elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        set_target_properties(TechEngineAPIServer PROPERTIES SERVER_API_DLL "${OUTPUT}/lib/release/TechEngineAPIServer.dll")
    else ()
        message(FATAL_ERROR "CMAKE_BUILD_TYPE is not set or is not recognized. Defaulting to Release.")
    endif ()
endif ()