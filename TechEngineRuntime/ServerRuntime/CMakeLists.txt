file(GLOB_RECURSE TechEngineServerRuntime_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineServerRuntime_Headers "src/*.hpp")

set(TechEngineServerRuntime_Include_Dirs "")

foreach (_headerFile ${TechEngineClientRuntime_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineServerRuntime_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineServerRuntime_Include_Dirs)

add_executable(TechEngineServerRuntime ${TechEngineServerRuntime_Sources} ${TechEngineServerRuntime_Headers})

#Setup target's output directory
if (WIN32) # WINDOWS
    set(TECH_ENGINE_SERVER_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/TechEngineRuntime/TechEngineServerRuntime")
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

target_link_directories(TechEngineServerRuntime PUBLIC ${TechEngineServerRuntime_Include_Dirs})
target_link_libraries(TechEngineServerRuntime TechEngineServer TechEngineCoreServer)
set_target_properties(TechEngineClientRuntime PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TechEngineServerRuntime/archive"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TechEngineServerRuntime/libs"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TechEngineServerRuntime/runtime"
)

#Copy dlls to output directory
add_custom_command(TARGET TechEngineServerRuntime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_RUNTIME_DLLS:TechEngineServerRuntime>
        $<TARGET_FILE_DIR:TechEngineServerRuntime>
        COMMAND_EXPAND_LISTS)
