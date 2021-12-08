file(GLOB_RECURSE Game_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE Game_Headers "src/*.hpp")

set(Game_Include_Dirs "")

foreach (_headerFile ${Game_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND Game_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES Game_Include_Dirs)

include_directories("../engine/src")

add_executable(Client ${Game_Sources} ${Game_Headers})

target_link_directories(Client PRIVATE ${Game_Include_Dirs})
target_link_libraries(Client engine)

