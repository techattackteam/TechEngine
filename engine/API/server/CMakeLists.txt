file(GLOB_RECURSE ENGINE_API_SERVER_SRC "src/*.cpp")
file(GLOB_RECURSE ENGINE_API_SERVER_HEADERS "include/*.hpp")

add_library(TechEngineAPIServer SHARED ${ENGINE_API_SERVER_SRC} ${ENGINE_API_SERVER_HEADERS})

target_link_libraries(TechEngineAPIServer PUBLIC TechEngineAPICommon TechEngineServer)
#target_compile_definitions(TechEngineAPIServer PRIVATE TECH_ENGINE_CLIENT_API_EXPORT)

target_include_directories(TechEngineAPIServer PUBLIC "src")
target_include_directories(TechEngineAPIServer PUBLIC "include")

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
    # Set target property to store the dll's path for editor and runtime to copy
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "")
        set_target_properties(TechEngineAPIServer PROPERTIES SERVER_API_DLL "${OUTPUT}/lib/debug/TechEngineAPIServer.dll")
    else ()
        set_target_properties(TechEngineAPIServer PROPERTIES SERVER_API_DLL "${OUTPUT}/lib/release/TechEngineAPIServer.dll")
    endif ()
endif ()