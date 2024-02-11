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
file(GLOB_RECURSE TechEngineAPI_include "src/script/*.hpp")
add_custom_command(TARGET TechEngineClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/TechEngineClient/src/ScriptingAPI" "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/TechEngineAPI/include"
        COMMAND_EXPAND_LISTS)

file(GLOB_RECURSE CPP_FILES "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/TechEngineAPI/include/*.cpp")

foreach (_file ${CPP_FILES})
    if (EXISTS ${_file})
        add_custom_command(TARGET TechEngineClient
                COMMAND ${CMAKE_COMMAND} -E rm -f ${_file}
                COMMAND_EXPAND_LISTS)
    endif ()
endforeach ()