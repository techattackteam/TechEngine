
# include all source files from the src directory
file(GLOB RUNTIME_SERVER_SOURCES "src/*.cpp")
file(GLOB RUNTIME_SERVER_HEADERS "src/*.hpp")

# add the executable
add_executable(TechEngineRuntimeServer ${RUNTIME_SERVER_SOURCES} ${RUNTIME_SERVER_HEADERS})
target_include_directories(TechEngineRuntimeServer PUBLIC src)
target_link_libraries(TechEngineRuntimeServer TechEngineApp TechEngineServer)

# setup target's output directory
if (WIN32) # WINDOWS
    set_target_properties(TechEngineRuntimeServer PROPERTIES TECH_ENGINE_RUNTIME_SERVER_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/runtime/runtimes/server")
    get_target_property(OUTPUT TechEngineRuntimeServer TECH_ENGINE_RUNTIME_SERVER_OUTPUT_DIRECTORY)
    set_target_properties(TechEngineRuntimeServer PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
    )
    set_target_properties(TechEngineRuntimeServer PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
    )
endif ()

#Copy the dlls to the output directory
if (WIN32) # WINDOWS
    get_target_property(ServerDLL TechEngineServer SERVER_DLL)
    add_custom_command(TARGET TechEngineRuntimeServer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${ServerDLL}
            $<TARGET_FILE_DIR:TechEngineRuntimeServer>
    )
endif ()