file(GLOB_RECURSE TechEngineServerRuntime_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineServerRuntime_Headers "src/*.hpp")

add_executable(TechEngineServerRuntime ${TechEngineServerRuntime_Sources} ${TechEngineServerRuntime_Headers})
#Setup target's output directory
if (WIN32) # WINDOWS
    set(TECH_ENGINE_SERVER_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/TechEngineServerRuntime/")
    set_target_properties(TechEngineServerRuntime PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/debug"
    )
    set_target_properties(TechEngineServerRuntime PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/release"
    )
endif ()

include_directories(src)

target_include_directories(TechEngineServerRuntime PUBLIC "src")
target_link_libraries(TechEngineServerRuntime TechEngineServer)

#Copy dlls to output directory
add_custom_command(TARGET TechEngineServerRuntime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_RUNTIME_DLLS:TechEngineServerRuntime>
        $<TARGET_FILE_DIR:TechEngineServerRuntime>
        COMMAND_EXPAND_LISTS)
