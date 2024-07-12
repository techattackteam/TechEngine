file(GLOB_RECURSE TechEngineClientRuntime_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineClientRuntime_Headers "src/*.hpp")

set(TechEngineClientRuntime_Include_Dirs "")

foreach (_headerFile ${TechEngineClientRuntime_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineClientRuntime_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineClientRuntime_Include_Dirs)

add_executable(TechEngineClientRuntime ${TechEngineClientRuntime_Sources} ${TechEngineClientRuntime_Headers})

#Setup target's output directory
if (WIN32) # WINDOWS
    set(TECH_ENGINE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/TechEngineRuntime/TechEngineClientRuntime")
    set_target_properties(TechEngineClientRuntime PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_RUNTIME_OUTPUT_DIRECTORY}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_RUNTIME_OUTPUT_DIRECTORY}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_RUNTIME_OUTPUT_DIRECTORY}/debug"
    )
    set_target_properties(TechEngineClientRuntime PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_RUNTIME_OUTPUT_DIRECTORY}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_RUNTIME_OUTPUT_DIRECTORY}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_RUNTIME_OUTPUT_DIRECTORY}/release"
    )
endif ()

target_link_directories(TechEngineClientRuntime PRIVATE ${TechEngineClientRuntime_Include_Dirs})
target_link_libraries(TechEngineClientRuntime TechEngineClient TechEngineCoreClient)
set_target_properties(TechEngineClientRuntime PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TechEngineClientRuntime/archive"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TechEngineClientRuntime/libs"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TechEngineClientRuntime/runtime"
)
target_compile_definitions(TechEngineClientRuntime PUBLIC
        $<$<CONFIG:Debug>:TE_DEBUG>
        $<$<CONFIG:Release>:TE_RELEASE>
)

#Copy dll files to build directory
add_custom_command(TARGET TechEngineClientRuntime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_RUNTIME_DLLS:TechEngineClientRuntime>
        $<TARGET_FILE_DIR:TechEngineClientRuntime>
        COMMAND_EXPAND_LISTS)
