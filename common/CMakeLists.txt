file(GLOB_RECURSE Common_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE Common_Headers "src/*.hpp")

set(Common_Include_Dirs "")
foreach (_headerFile ${Common_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND Common_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES Common_Include_Dirs)

include_directories("../TechEngineCore/src/external")
add_library(common STATIC ${Common_Sources} ${Common_Headers})

target_include_directories(common PUBLIC "../TechEngineCore/src/external")
target_include_directories(common PUBLIC "src")
target_link_libraries(common TechEngineCore)