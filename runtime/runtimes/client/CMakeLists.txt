
# include all source files from the src directory
file(GLOB RUNTIME_CLIENT_SOURCES "src/*.cpp")
file(GLOB RUNTIME_CLIENT_HEADERS "src/*.hpp")

# add the executable
add_executable(TechEngineRuntimeClient ${RUNTIME_CLIENT_SOURCES} ${RUNTIME_CLIENT_HEADERS})
target_include_directories(TechEngineRuntimeClient PUBLIC src)
target_link_libraries(TechEngineRuntimeClient TechEngineApp TechEngineCore TechEngineClient)

# setup target's output directory
if (WIN32) # WINDOWS
    set_target_properties(TechEngineRuntimeClient PROPERTIES TECH_ENGINE_RUNTIME_CLIENT_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/runtime/runtimes/client")
    get_target_property(OUTPUT TechEngineRuntimeClient TECH_ENGINE_RUNTIME_CLIENT_OUTPUT_DIRECTORY)
    set_target_properties(TechEngineRuntimeClient PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
    )
    set_target_properties(TechEngineRuntimeClient PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
    )
endif ()

#Copy the dlls to the output directory
if (WIN32) # WINDOWS
    get_target_property(ClientDLL TechEngineClient CLIENT_DLL)
    add_custom_command(TARGET TechEngineRuntimeClient POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${ClientDLL}
            $<TARGET_FILE_DIR:TechEngineRuntimeClient>
    )
endif ()