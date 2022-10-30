file(GLOB_RECURSE TechEngineEditor_Sources "src/*.cpp")
file(GLOB_RECURSE TechEngineEditor_Headers "src/*.hpp")

find_package(imgui CONFIG REQUIRED)

add_executable(TechEngineEditor ${TechEngineEditor_Sources} ${TechEngineEditor_Headers})

target_include_directories(TechEngineEditor PRIVATE "src")
target_link_libraries(TechEngineEditor TechEngineGame imgui::imgui)

add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/TechEngineGame/res ${CMAKE_BINARY_DIR}/TechEngineGame/res)


#Copy Engine lib for user code
file(GLOB_RECURSE TechEngineAPI_libs "${CMAKE_BINARY_DIR}/TechEngineGame/archive/TechEngineGame.lib" "${CMAKE_BINARY_DIR}/TechEngineCore/archive/TechEngineCore.lib")
add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${TechEngineAPI_libs} $<TARGET_FILE_DIR:TechEngineEditor>/TechEngineAPI/lib
        COMMAND_EXPAND_LISTS)

#Copy Engine includes for user code
#For now it will include everything -> subject to change
file(GLOB_RECURSE TechEngineAPI_include "${CMAKE_CURRENT_SOURCE_DIR}/../TechEngineGame/src")
add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/../TechEngineGame/src $<TARGET_FILE_DIR:TechEngineEditor>/TechEngineAPI/include/TechEngineGame
        COMMAND_EXPAND_LISTS)


file(GLOB_RECURSE TechEngineAPI_include "${CMAKE_CURRENT_SOURCE_DIR}/../TechEngineCore/src")
add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/../TechEngineCore/src $<TARGET_FILE_DIR:TechEngineEditor>/TechEngineAPI/include/TechEngineCore
        COMMAND_EXPAND_LISTS)

file(GLOB_RECURSE CORE_FILES "${CMAKE_BINARY_DIR}/TechEngineEditor/TechEngineAPI/include/TechEngineCore/*.cpp")
file(GLOB_RECURSE GAME_FILES "${CMAKE_BINARY_DIR}/TechEngineEditor/TechEngineAPI/include/TechEngineGame/*.cpp")

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