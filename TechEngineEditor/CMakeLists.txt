file(GLOB_RECURSE TechEngineEditor_Sources "src/*.cpp")
file(GLOB_RECURSE TechEngineEditor_Headers "src/*.hpp")

find_package(imgui CONFIG REQUIRED)

add_executable(TechEngineEditor ${TechEngineEditor_Sources} ${TechEngineEditor_Headers})

target_include_directories(TechEngineEditor PRIVATE "../TechEngineGame/src/external")
target_include_directories(TechEngineEditor PRIVATE "src")
target_link_libraries(TechEngineEditor TechEngineGame imgui::imgui)

add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/TechEngineGame/res ${CMAKE_BINARY_DIR}/TechEngineGame/res)

add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_RUNTIME_DLLS:TechEngineEditor> $<TARGET_FILE_DIR:TechEngineEditor>
        COMMAND_EXPAND_LISTS)

add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE_DIR:Game> $<TARGET_FILE_DIR:TechEngineEditor>/runtime
        COMMAND_EXPAND_LISTS)