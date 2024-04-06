cmake_minimum_required(VERSION 3.24)

project(UserProject)

set(CMAKE_CXX_STANDARD 20)
set(TechEngineCoreLIB "TechEngineCoreLIB-placeholder" CACHE STRING "File path for the TechEngineCore lib")
set(TechEngineClientLIB "TechEngineClientLIB-placeholder" CACHE STRING "File path for the TechEngineClient lib")

file(GLOB_RECURSE UserScripts_Sources CONFIGURE_DEPENDS "../../Assets/*.cpp")
file(GLOB_RECURSE UserScripts_Headers "../../Assets/*.hpp")
foreach (_headerFile ${TechEngineAPI_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineAPI_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineAPIInclude_Dirs)

add_library(UserScripts SHARED ${UserScripts_Sources} ${UserScripts_Headers})

if (WIN32) # WINDOWS
    set(TECH_ENGINE_API_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../scripts/build")
    set_target_properties(UserScripts PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/debug"
    )
    set_target_properties(UserScripts PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/release"
    )
    set_target_properties(UserScripts PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/releaseWithDebug"
            LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/releaseWithDebug"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/releaseWithDebug"
    )
endif ()
target_include_directories(UserScripts PUBLIC
        "../TechEngineAPI/include/TechEngineCore"
        "../TechEngineAPI/include/TechEngineClient"
        "../TechEngineAPI/include"
        "../dependencies/glm"
        "../dependencies/yaml-cpp/include")
target_link_libraries(UserScripts ${TechEngineCoreLIB} ${TechEngineClientLIB})
