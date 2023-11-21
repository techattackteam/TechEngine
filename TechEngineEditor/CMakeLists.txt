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
target_link_libraries(TechEngineEditor TechEngineClient imgui::imgui imguizmo::imguizmo assimp::assimp)

add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/TechEngineClient/res ${CMAKE_BINARY_DIR}/TechEngineClient/res)

#Copy Engine lib for user code
file(GLOB_RECURSE TechEngineAPI_libs "${PROJECT_SOURCE_DIR}/bin/TechEngineClient/debug/TechEngineClient.lib" "${PROJECT_SOURCE_DIR}/bin/TechEngineCore/debug/TechEngineCore.lib")
add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${TechEngineAPI_libs} $<TARGET_FILE_DIR:TechEngineEditor>/TechEngineAPI/lib
        COMMAND_EXPAND_LISTS)

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

add_custom_command(
        TARGET TechEngineEditor
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -P "CopyConfigurationData$<CONFIG>.cmake"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

#Copy Engine includes for user code
#For now it will include everything -> subject to change
file(GLOB_RECURSE TechEngineAPI_include "${CMAKE_CURRENT_SOURCE_DIR}/../TechEngineClient/src")
add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/../TechEngineClient/src $<TARGET_FILE_DIR:TechEngineEditor>/TechEngineAPI/include/TechEngineGame
        COMMAND_EXPAND_LISTS)

file(GLOB_RECURSE TechEngineAPI_include "${CMAKE_CURRENT_SOURCE_DIR}/../TechEngineCore/src")
add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/../TechEngineCore/src $<TARGET_FILE_DIR:TechEngineEditor>/TechEngineAPI/include/TechEngineCore
        COMMAND_EXPAND_LISTS)

file(GLOB_RECURSE CORE_FILES "${CMAKE_BINARY_DIR}/TechEngineEditor/TechEngineAPI/include/TechEngineCore/*.cpp")
file(GLOB_RECURSE GAME_FILES "${CMAKE_BINARY_DIR}/TechEngineEditor/TechEngineAPI/include/TechEngineClient/*.cpp")

if (EXISTS CORE_FILES)
    add_custom_command(TARGET TechEngineEditor
            COMMAND ${CMAKE_COMMAND} -E rm -f ${GAME_FILES}
            COMMAND_EXPAND_LISTS)
endif ()

if (EXISTS GAME_FILES)
    add_custom_command(TARGET TechEngineEditor
            COMMAND ${CMAKE_COMMAND} -E rm -f ${CORE_FILES}
            COMMAND_EXPAND_LISTS)
endif ()

#copy engine's runtime to build directory
add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        $<TARGET_FILE_DIR:TechEngineRuntime> ${CMAKE_BINARY_DIR}/TechEngineEditor/runtime
        COMMAND_EXPAND_LISTS)