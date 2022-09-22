file(GLOB_RECURSE Server_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE Server_Headers "src/*.hpp")

set(Game_Include_Dirs "")

foreach (_headerFile ${Server_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND Game_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES Game_Include_Dirs)

include_directories("../TechEngineServer/src/external")
include_directories("../common")

add_executable(Server ${Server_Sources} ${Server_Headers})

target_link_directories(Server PRIVATE ${Server_Include_Dirs})
target_link_libraries(Server TechEngineServer common)