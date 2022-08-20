file(GLOB_RECURSE TechEngineGame_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineGame_Headers "src/*.hpp")

find_package(GLEW REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_definitions(-DENGINE_BUILD_DLL)

foreach (_headerFile ${TechEngineGame_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineGame_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineGame_Include_Dirs)

add_library(TechEngineGame SHARED ${TechEngineGame_Sources} ${TechEngineGame_Headers})

target_include_directories(TechEngineGame PUBLIC "../TechEngineCore/src/external")
target_include_directories(TechEngineGame PRIVATE "src")
target_link_libraries(TechEngineGame TechEngineCore imgui::imgui GLEW::GLEW glfw glm::glm -DENGINE_EDITOR)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(TechEngineGame PUBLIC ENGINE_EDITOR)
endif ()

add_custom_target(copy_shaders ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/TechEngineGame/res ${CMAKE_CURRENT_BINARY_DIR}/res)
