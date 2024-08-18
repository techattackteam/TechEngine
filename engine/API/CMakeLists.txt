
file(GLOB_RECURSE ENGINE_API_SRC "src/*.cpp")
file(GLOB_RECURSE ENGINE_API_HEADERS "include/*.hpp")

add_library(TechEngineAPI SHARED ${ENGINE_API_SRC} ${ENGINE_API_HEADERS})

target_include_directories(TechEngineAPI PUBLIC "src")
target_include_directories(TechEngineAPI PUBLIC "include")

target_link_libraries(TechEngineAPI PUBLIC TechEngineClient)

target_compile_definitions(TechEngineAPI PRIVATE TECH_ENGINE_CLIENT_API_EXPORT)
target_compile_definitions(TechEngineAPI PRIVATE GLM_ENABLE_EXPERIMENTAL)

#Setup target's output directory
if (WIN32) # WINDOWS
    set(OUTPUT "${PROJECT_SOURCE_DIR}/bin/engine/api")

    # Set target property to store the dll's path for editor and runtime to copy
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "")
        set_target_properties(TechEngineAPI PROPERTIES API_OUTPUT_DIRECTORY "${OUTPUT}")
    else ()
        set_target_properties(TechEngineAPI PROPERTIES API_OUTPUT_DIRECTORY "${OUTPUT}")
    endif ()

    # Set the output directory
    set_target_properties(TechEngineAPI PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/lib/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/lib/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/lib/debug"
    )
    set_target_properties(TechEngineAPI PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/lib/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/lib/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/lib/release"
    )


    # Empty the include directory
    add_custom_command(TARGET TechEngineAPI POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${OUTPUT}/include"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT}/include"
    )

    # Copy API files to API directory
    add_custom_command(TARGET TechEngineAPI POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/engine/API/include"
            "${OUTPUT}/include"
    )
endif ()
