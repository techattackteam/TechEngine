
# include all source files from the src directory
file(GLOB EDITOR_SOURCES "src/*.cpp")
file(GLOB EDITOR_HEADERS "src/*.hpp")

# add the executable
add_executable(TechEngineEditor ${EDITOR_SOURCES} ${EDITOR_HEADERS})
target_include_directories(TechEngineEditor PUBLIC src)
target_link_libraries(TechEngineEditor TechEngineApp TechEngineClient TechEngineServer)

# setup target's output directory
if (WIN32) # WINDOWS
    set_target_properties(TechEngineEditor PROPERTIES TECH_ENGINE_EDITOR_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/runtime/editor/")
    get_target_property(OUTPUT TechEngineEditor TECH_ENGINE_EDITOR_OUTPUT_DIRECTORY)
    set_target_properties(TechEngineEditor PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
    )
    set_target_properties(TechEngineEditor PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
    )
endif ()

# Copy the dlls to the output directory
if (WIN32) # WINDOWS
    get_target_property(ClientDLL TechEngineClient CLIENT_DLL)
    get_target_property(ServerDLL TechEngineServer SERVER_DLL)
    add_custom_command(TARGET TechEngineEditor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${ClientDLL}
            $<TARGET_FILE_DIR:TechEngineEditor>
    )
    add_custom_command(TARGET TechEngineEditor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${ServerDLL}
            $<TARGET_FILE_DIR:TechEngineEditor>
    )
endif ()

# Copy runtime to editor
if (WIN32)
    add_custom_target(Runtimes ALL
            DEPENDS TechEngineRuntimeClient TechEngineRuntimeServer
    )

    # Add the runtime dependencies to the editor
    add_dependencies(TechEngineEditor Runtimes)

    get_target_property(ClientRuntime TechEngineRuntimeClient RUNTIME_CLIENT_OUTPUT_DIRECTORY)
    # Add commands to copy the runtime files to the editor directory
    add_custom_command(TARGET TechEngineEditor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${ClientRuntime}
            $<TARGET_FILE_DIR:TechEngineEditor>/runtimes/client
    )

    get_target_property(ServerRuntime TechEngineRuntimeServer RUNTIME_SERVER_OUTPUT_DIRECTORY)
    # Add commands to copy the runtime files to the editor directory
    add_custom_command(TARGET TechEngineEditor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${ServerRuntime}
            $<TARGET_FILE_DIR:TechEngineEditor>/runtimes/server
    )


endif ()