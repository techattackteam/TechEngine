file(GLOB_RECURSE TechEngineServer_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineServer_Headers "src/*.hpp")

add_executable(TechEngineServer ${TechEngineServer_Sources} ${TechEngineServer_Headers})
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

#Copy API include files to project template
set(TECH_ENGINE_SERVER_API_DIRECTORY "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Server/TechEngineAPI/include/TechEngineServer")
set(TECH_ENGINE_SERVER_API_FILES
        "${PROJECT_SOURCE_DIR}/TechEngineServer/src/core/Client.hpp" #Placeholder
        "${PROJECT_SOURCE_DIR}/TechEngineServer/src/events"
)

#Clear TechEngineServer API directory
file(REMOVE_RECURSE "${TECH_ENGINE_SERVER_API_DIRECTORY}")

foreach (entry ${TECH_ENGINE_SERVER_API_FILES})
    if (IS_DIRECTORY ${entry})
        get_filename_component(folder_name ${entry} NAME)
        add_custom_command(TARGET TechEngineServer POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E rm -rf "${TECH_ENGINE_SERVER_API_DIRECTORY}/${folder_name}"
                COMMAND_EXPAND_LISTS)
        message(STATUS "Removed ${TECH_ENGINE_SERVER_API_DIRECTORY}/${folder_name}")
        add_custom_command(TARGET TechEngineServer POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${entry}" "${TECH_ENGINE_SERVER_API_DIRECTORY}/${folder_name}"
                COMMAND_EXPAND_LISTS)
        add_custom_command(TARGET TechEngineServer POST_BUILD
                COMMAND powershell -Command "Get-ChildItem -Path ${TECH_ENGINE_SERVER_API_DIRECTORY}/${folder_name} -Filter *.cpp -Recurse | Remove-Item -Force"
                COMMAND_EXPAND_LISTS)
    else ()
        get_filename_component(folder_name ${entry} DIRECTORY)
        get_filename_component(folder_name ${folder_name} NAME)
        if (NOT EXISTS "${TECH_ENGINE_SERVER_API_DIRECTORY}/${folder_name}")
            file(MAKE_DIRECTORY "${TECH_ENGINE_SERVER_API_DIRECTORY}/${folder_name}")
        endif ()
        add_custom_command(TARGET TechEngineServer PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${entry}" "${TECH_ENGINE_SERVER_API_DIRECTORY}/${folder_name}"
                COMMAND_EXPAND_LISTS)
    endif ()
endforeach ()


#Copy .lib to project template
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (NOT EXISTS "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Server/TechEngineAPI/lib/debug")
        file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Server/TechEngineAPI/lib/debug")
    endif ()
    add_custom_command(TARGET TechEngineServer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/debug/TechEngineServer.lib"
            "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Server/TechEngineAPI/lib/debug"
            COMMAND_EXPAND_LISTS)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    if (NOT EXISTS "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Server/TechEngineAPI/lib/release")
        file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Server/TechEngineAPI/lib/release")
    endif ()
    add_custom_command(TARGET TechEngineServer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/release/TechEngineServer.lib"
            "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Server/TechEngineAPI/lib/release"
            COMMAND_EXPAND_LISTS)
endif ()