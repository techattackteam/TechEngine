file(GLOB_RECURSE TechEngineClient_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineClient_Headers "src/*.hpp")

find_package(GLEW REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

add_definitions(-DENGINE_BUILD_DLL)

foreach (_headerFile ${TechEngineClient_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineClient_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineClient_Include_Dirs)

add_library(TechEngineClient SHARED ${TechEngineClient_Sources} ${TechEngineClient_Headers})

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

target_include_directories(TechEngineClient PUBLIC "src")
target_link_libraries(TechEngineClient TechEngineCore GLEW::GLEW glfw glm::glm yaml-cpp)

add_custom_command(TARGET TechEngineClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/TechEngineClient/res $<TARGET_FILE_DIR:TechEngineEditor>/resources
        COMMAND_EXPAND_LISTS)

add_custom_command(TARGET TechEngineClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_RUNTIME_DLLS:TechEngineEditor>
        $<TARGET_FILE_DIR:TechEngineEditor>
        COMMAND_EXPAND_LISTS)

