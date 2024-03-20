file(GLOB_RECURSE TechEngineClient_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineClient_Headers "src/*.hpp")

find_package(GLEW REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
#find_package(Stb REQUIRED)

foreach (_headerFile ${TechEngineClient_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineClient_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineClient_Include_Dirs)

add_library(TechEngineClient SHARED ${TechEngineClient_Sources} ${TechEngineClient_Headers})

add_compile_definitions(TECH_ENGINE_API_EXPORTS)

#Setup target's output directory
if (WIN32) # WINDOWS
    set(TECH_ENGINE_CLIENT_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/TechEngineClient/")
    set_target_properties(TechEngineClient PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_CLIENT_OUTPUT_DIRECTORY}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_CLIENT_OUTPUT_DIRECTORY}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_CLIENT_OUTPUT_DIRECTORY}/debug"
    )
    set_target_properties(TechEngineClient PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_CLIENT_OUTPUT_DIRECTORY}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_CLIENT_OUTPUT_DIRECTORY}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_CLIENT_OUTPUT_DIRECTORY}/release"
    )
endif ()
set(Stb_INCLUDE_DIR "C:\\dev\\TechEngine\\dependencies\\vcpkg\\packages\\stb_x64-windows\\include")
target_include_directories(TechEngineClient PUBLIC ${Stb_INCLUDE_DIR})
target_include_directories(TechEngineClient PUBLIC "src")
target_link_libraries(TechEngineClient PUBLIC TechEngineCore GLEW::GLEW glfw glm::glm yaml-cpp)


add_custom_command(TARGET TechEngineClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/TechEngineClient/res $<TARGET_FILE_DIR:TechEngineEditor>/resources
        COMMAND_EXPAND_LISTS)

add_custom_command(TARGET TechEngineClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_RUNTIME_DLLS:TechEngineEditor>
        $<TARGET_FILE_DIR:TechEngineEditor>
        COMMAND_EXPAND_LISTS)

#Copy API include files to project template
set(TECH_ENGINE_CLIENT_API_DIRECTORY "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/TechEngineAPI/include/TechEngineClient")
set(TECH_ENGINE_CLIENT_API_FILES
        "${PROJECT_SOURCE_DIR}/TechEngineClient/src/core/Key.hpp"
        "${PROJECT_SOURCE_DIR}/TechEngineClient/src/core/Mouse.hpp"
        "${PROJECT_SOURCE_DIR}/TechEngineClient/src/material/Material.hpp"
        "${PROJECT_SOURCE_DIR}/TechEngineClient/src/components"
        "${PROJECT_SOURCE_DIR}/TechEngineClient/src/events"
        "${PROJECT_SOURCE_DIR}/TechEngineClient/src/script"
        "${PROJECT_SOURCE_DIR}/TechEngineClient/src/scriptingAPI"
        "${PROJECT_SOURCE_DIR}/TechEngineClient/src/texture"
)

#Clear TechEngineClient API directory
file(REMOVE_RECURSE "${TECH_ENGINE_CLIENT_API_DIRECTORY}")


foreach (entry ${TECH_ENGINE_CLIENT_API_FILES})
    if (IS_DIRECTORY ${entry})
        get_filename_component(folder_name ${entry} NAME)
        add_custom_command(TARGET TechEngineClient POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E rm -rf "${TECH_ENGINE_CLIENT_API_DIRECTORY}/${folder_name}"
                COMMAND_EXPAND_LISTS)
        message(STATUS "Removed ${TECH_ENGINE_CLIENT_API_DIRECTORY}/${folder_name}")
        add_custom_command(TARGET TechEngineClient POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${entry}" "${TECH_ENGINE_CLIENT_API_DIRECTORY}/${folder_name}"
                COMMAND_EXPAND_LISTS)
        add_custom_command(TARGET TechEngineClient POST_BUILD
                COMMAND powershell -Command "Get-ChildItem -Path ${TECH_ENGINE_CLIENT_API_DIRECTORY}/${folder_name} -Filter *.cpp -Recurse | Remove-Item -Force"
                COMMAND_EXPAND_LISTS)
    else ()
        get_filename_component(folder_name ${entry} DIRECTORY)
        get_filename_component(folder_name ${folder_name} NAME)
        if (NOT EXISTS "${TECH_ENGINE_CLIENT_API_DIRECTORY}/${folder_name}")
            file(MAKE_DIRECTORY "${TECH_ENGINE_CLIENT_API_DIRECTORY}/${folder_name}")
        endif ()
        add_custom_command(TARGET TechEngineClient PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${entry}" "${TECH_ENGINE_CLIENT_API_DIRECTORY}/${folder_name}"
                COMMAND_EXPAND_LISTS)
    endif ()
endforeach ()


#Copy DLLS to project template
add_custom_command(TARGET TechEngineClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${TECH_ENGINE_CLIENT_OUTPUT_DIRECTORY}/debug/TechEngineClient.lib"
        "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/TechEngineAPI/lib"
        COMMAND_EXPAND_LISTS)