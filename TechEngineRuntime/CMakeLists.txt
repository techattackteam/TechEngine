file(GLOB_RECURSE TechEngineRuntime_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineRuntime_Headers "src/*.hpp")

set(TechEngineRuntime_Include_Dirs "")

foreach (_headerFile ${TechEngineRuntime_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineRuntime_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineRuntime_Include_Dirs)

add_executable(TechEngineRuntime ${TechEngineRuntime_Sources} ${TechEngineRuntime_Headers})

#Setup target's output directory
if (WIN32) # WINDOWS
    set(TECH_ENGINE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/TechEngineRuntime/")
    set_target_properties(TechEngineRuntime PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_RUNTIME_OUTPUT_DIRECTORY}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_RUNTIME_OUTPUT_DIRECTORY}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_RUNTIME_OUTPUT_DIRECTORY}/debug"
            )
    set_target_properties(TechEngineRuntime PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_RUNTIME_OUTPUT_DIRECTORY}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_RUNTIME_OUTPUT_DIRECTORY}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_RUNTIME_OUTPUT_DIRECTORY}/release"
            )
endif ()

target_link_directories(TechEngineRuntime PRIVATE ${TechEngineRuntime_Include_Dirs})
target_link_libraries(TechEngineRuntime TechEngineClient)

set_target_properties(TechEngineRuntime PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TechEngineRuntime/archive"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TechEngineRuntime/libs"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TechEngineRuntime/runtime"
        )

add_custom_command(TARGET TechEngineRuntime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:TechEngineRuntime> $<TARGET_FILE_DIR:TechEngineRuntime>
        COMMAND_EXPAND_LISTS)

add_custom_command(TARGET TechEngineRuntime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        $<TARGET_FILE_DIR:TechEngineRuntime> $<TARGET_FILE_DIR:TechEngineEditor>/runtime
        COMMAND_EXPAND_LISTS)
