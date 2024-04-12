file(GLOB_RECURSE TechEngineServer_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineServer_Headers "src/*.hpp")

add_executable(TechEngineServer ${TechEngineServer_Sources} ${TechEngineServer_Headers})
add_compile_definitions(TECH_ENGINE_API_EXPORTS)
#Setup target's output directory
if (WIN32) # WINDOWS
    set(TECH_ENGINE_SERVER_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/TechEngineServer/")
    set_target_properties(TechEngineServer PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/debug"
    )
    set_target_properties(TechEngineServer PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/release"
    )
endif ()

include_directories(src)

target_include_directories(TechEngineServer PUBLIC "src")
target_link_libraries(TechEngineServer TechEngineCore)

#Copy dlls to output directory
add_custom_command(TARGET TechEngineServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_RUNTIME_DLLS:TechEngineServer>
        $<TARGET_FILE_DIR:TechEngineServer>
        COMMAND_EXPAND_LISTS)