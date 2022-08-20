file(GLOB_RECURSE Game_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE Game_Headers "src/*.hpp")

set(Game_Include_Dirs "")

foreach (_headerFile ${Game_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND Game_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES Game_Include_Dirs)

include_directories("../TechEngineGame/src/external")

add_executable(Game ${Game_Sources} ${Game_Headers})

target_link_directories(Game PRIVATE ${Game_Include_Dirs})
target_link_libraries(Game TechEngineGame)

add_custom_command(TARGET Game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:Game> $<TARGET_FILE_DIR:Game>
        COMMAND_EXPAND_LISTS)