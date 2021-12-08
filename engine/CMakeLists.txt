file(GLOB_RECURSE Engine_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE Engine_Headers "src/*.hpp")

find_package(GLEW REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)


foreach (_headerFile ${Engine_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND Engine_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES Engine_Include_Dirs)


add_library(engine SHARED ${Engine_Sources} ${Engine_Headers})
target_link_libraries(engine imgui::imgui GLEW::GLEW glfw glm::glm)

add_custom_target(copy_shaders ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/engine/res/ ${CMAKE_CURRENT_BINARY_DIR}/res)