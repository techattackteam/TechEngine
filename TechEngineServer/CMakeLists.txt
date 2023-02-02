file(GLOB_RECURSE TechEngineServer_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineServer_Headers "src/*.hpp")

add_definitions(-DENGINE_BUILD_DLL)

foreach (_headerFile ${TechEngineServer_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineServer_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineServer_Include_Dirs)

add_library(TechEngineServer STATIC ${TechEngineServer_Sources} ${TechEngineServer_Headers})

#Setup target's output directory
if (WIN32) # WINDOWS
    set(TECH_ENGINE_SERVER_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/TechEngineServer/")
    set_target_properties(TechEngineCore PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/debug"
            )
    set_target_properties(TechEngineCore PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_SERVER_OUTPUT_DIRECTORY}/release"
            )
endif ()

include_directories(src)

target_include_directories(TechEngineServer PUBLIC "src")
target_link_libraries(TechEngineServer TechEngineCore)

