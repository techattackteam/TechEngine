file(GLOB_RECURSE TechEngineEditor_Sources "src/*.cpp")
file(GLOB_RECURSE TechEngineEditor_Headers "src/*.hpp")

find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

add_executable(TechEngineEditor ${TechEngineEditor_Sources} ${TechEngineEditor_Headers})

if (WIN32) # WINDOWS
    set(TECH_ENGINE_EDITOR_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/TechEngineEditor/")
    set_target_properties(TechEngineEditor PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_EDITOR_OUTPUT_DIRECTORY}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_EDITOR_OUTPUT_DIRECTORY}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_EDITOR_OUTPUT_DIRECTORY}/debug"
    )
    set_target_properties(TechEngineEditor PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_EDITOR_OUTPUT_DIRECTORY}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_EDITOR_OUTPUT_DIRECTORY}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_EDITOR_OUTPUT_DIRECTORY}/release"
    )
endif ()

target_include_directories(TechEngineEditor PRIVATE "src")
target_link_libraries(TechEngineEditor PUBLIC TechEngineClient imgui::imgui imguizmo::imguizmo assimp::assimp)

#Copy project template
file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/CopyConfigurationData$<CONFIG>.cmake"
        CONTENT
        "if (NOT EXISTS \"$<TARGET_FILE_DIR:TechEngineEditor>/projectTemplate\")
                execute_process(
                     COMMAND \"${CMAKE_COMMAND}\" -E copy_directory
                         \"${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate\"
                    \"$<TARGET_FILE_DIR:TechEngineEditor>/projectTemplate\"
                )
            endif()
         if (NOT EXISTS \"$<TARGET_FILE_DIR:TechEngineEditor>/scriptTemplate\")
                execute_process(
                     COMMAND \"${CMAKE_COMMAND}\" -E copy_directory
                         \"${PROJECT_SOURCE_DIR}/TechEngineTemplates/ScriptTemplate\"
                    \"$<TARGET_FILE_DIR:TechEngineEditor>/scriptTemplate\"
                )
            endif()"
)

#Copy dll files to build directory
add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_RUNTIME_DLLS:TechEngineEditor>
        $<TARGET_FILE_DIR:TechEngineEditor>
        COMMAND_EXPAND_LISTS)

add_custom_command(
        TARGET TechEngineEditor
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -P "CopyConfigurationData$<CONFIG>.cmake"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

#Copy engine's runtime to build directory
add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        $<TARGET_FILE_DIR:TechEngineRuntime> $<TARGET_FILE_DIR:TechEngineEditor>/TechEngineRuntime
        COMMAND_EXPAND_LISTS)

add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        $<TARGET_FILE_DIR:TechEngineServer> $<TARGET_FILE_DIR:TechEngineEditor>/TechEngineServer
        COMMAND_EXPAND_LISTS)