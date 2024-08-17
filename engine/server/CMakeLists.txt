file(GLOB_RECURSE ENGINE_SERVER_SRC "src/*.cpp")
file(GLOB_RECURSE ENGINE_SERVER_HEADERS "src/*.hpp")

add_library(TechEngineServer SHARED ${ENGINE_SERVER_SRC} ${ENGINE_SERVER_HEADERS})

target_compile_definitions(TechEngineServer PRIVATE TECH_ENGINE_SERVER_EXPORT TECH_ENGINE_CORE_EXPORT)

target_include_directories(TechEngineServer PUBLIC src)

target_link_libraries(TechEngineServer PUBLIC TechEngineCore)

#Setup target's output directory
if (WIN32) # WINDOWS
    set(OUTPUT "${PROJECT_SOURCE_DIR}/bin/engine/server")

    # Set target property to store the dll's path for editor and runtime to copy
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "")
        set_target_properties(TechEngineServer PROPERTIES SERVER_DLL "${OUTPUT}/debug/TechEngineServer.dll")
    else ()
        set_target_properties(TechEngineServer PROPERTIES SERVER_DLL "${OUTPUT}/release/TechEngineServer.dll")
    endif ()

    # Set the output directory
    set_target_properties(TechEngineServer PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
    )
    set_target_properties(TechEngineServer PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
    )
endif ()

