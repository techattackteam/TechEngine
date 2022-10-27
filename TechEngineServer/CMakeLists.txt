file(GLOB_RECURSE TechEngineServer_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineServer_Headers "src/*.hpp")

add_definitions(-DENGINE_BUILD_DLL)

foreach (_headerFile ${TechEngineServer_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineServer_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineServer_Include_Dirs)

add_library(TechEngineServer SHARED ${TechEngineServer_Sources} ${TechEngineServer_Headers})

include_directories(src)

target_include_directories(TechEngineServer PUBLIC "src")
target_link_libraries(TechEngineServer TechEngineCore)

