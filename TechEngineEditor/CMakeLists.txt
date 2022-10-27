file(GLOB_RECURSE TechEngineEditor_Sources "src/*.cpp")
file(GLOB_RECURSE TechEngineEditor_Headers "src/*.hpp")

find_package(imgui CONFIG REQUIRED)

add_executable(TechEngineEditor ${TechEngineEditor_Sources} ${TechEngineEditor_Headers})

target_include_directories(TechEngineEditor PRIVATE "src")
target_link_libraries(TechEngineEditor TechEngineGame imgui::imgui)

add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/TechEngineGame/res ${CMAKE_BINARY_DIR}/TechEngineGame/res)




