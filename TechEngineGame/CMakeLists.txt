file(GLOB_RECURSE TechEngineGame_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineGame_Headers "src/*.hpp")

find_package(GLEW REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

add_definitions(-DENGINE_BUILD_DLL)

foreach (_headerFile ${TechEngineGame_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineGame_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineGame_Include_Dirs)

add_library(TechEngineGame SHARED ${TechEngineGame_Sources} ${TechEngineGame_Headers})

target_include_directories(TechEngineGame PUBLIC "../TechEngineScript/src")
target_include_directories(TechEngineGame PUBLIC "src")
target_link_libraries(TechEngineGame TechEngineCore GLEW::GLEW glfw glm::glm yaml-cpp)

set_target_properties(TechEngineGame PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TechEngineGame/archive"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TechEngineGame/libs"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TechEngineGame/runtime")


add_custom_command(TARGET TechEngineGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/TechEngineGame/res $<TARGET_FILE_DIR:TechEngineEditor>/resources
        COMMAND_EXPAND_LISTS)

add_custom_command(TARGET TechEngineGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_RUNTIME_DLLS:TechEngineEditor>
        $<TARGET_FILE_DIR:TechEngineEditor>
        COMMAND_EXPAND_LISTS)

