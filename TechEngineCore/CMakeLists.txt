file(GLOB_RECURSE TechEngineCore_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineCore_Headers "src/*.hpp")

find_package(Boost COMPONENTS serialization)
find_package(spdlog CONFIG REQUIRED)

add_definitions(-DENGINE_BUILD_DLL)

foreach (_headerFile ${TechEngineCore_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineCore_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineCore_Include_Dirs)

add_library(TechEngineCore SHARED ${TechEngineCore_Sources} ${TechEngineCore_Headers})


#Setup target's output directory
if (WIN32) # WINDOWS
    set(TECH_ENGINE_CORE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/TechEngineCore/")
    set_target_properties(TechEngineCore PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_CORE_OUTPUT_DIRECTORY}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_CORE_OUTPUT_DIRECTORY}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_CORE_OUTPUT_DIRECTORY}/debug"
            )
    set_target_properties(TechEngineCore PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_CORE_OUTPUT_DIRECTORY}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_CORE_OUTPUT_DIRECTORY}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_CORE_OUTPUT_DIRECTORY}/release"
            )
endif ()

target_include_directories(TechEngineCore PUBLIC "src")
target_link_libraries(TechEngineCore Boost::boost Boost::serialization spdlog::spdlog)
