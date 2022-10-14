file(GLOB_RECURSE TechEngineScript_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineScript_Headers "src/*.hpp")


add_library(TechEngineScript SHARED ${TechEngineScript_Sources} ${TechEngineScript_Headers})

target_include_directories(TechEngineScript PRIVATE "../TechEngineCore/src")
target_include_directories(TechEngineScript PRIVATE "src")
target_link_libraries(TechEngineScript TechEngineCore)

add_definitions(-DSCRIPT_BUILD_DLL)


add_custom_command(TARGET TechEngineScript POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:TechEngineScript> $<TARGET_FILE_DIR:TechEngineEditor>
        COMMAND_EXPAND_LISTS)
