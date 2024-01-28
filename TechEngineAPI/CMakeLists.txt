file(GLOB_RECURSE TechEngineAPI_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineAPI_Headers "src/*.hpp")

foreach (_headerFile ${TechEngineAPI_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineCore_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineCore_Include_Dirs)


add_library(TechEngineAPI STATIC ${TechEngineAPI_Sources} ${TechEngineAPI_Headers})

if (WIN32) # WINDOWS
    set(TECH_ENGINE_CORE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/TechEngineAPI/lib")
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


target_include_directories(TechEngineAPI PUBLIC "src" })
target_link_libraries(TechEngineAPI PUBLIC TechEngineClient)


#Copy header files to include directory
file(GLOB_RECURSE TechEngineAPI_include "src")
add_custom_command(TARGET TechEngineAPI POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/TechEngineAPI/src" "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/TechEngineAPI/include"
        COMMAND_EXPAND_LISTS)

file(GLOB_RECURSE CPP_FILES "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/TechEngineAPI/include/*.cpp")

foreach(_file ${CPP_FILES})
    if (EXISTS ${_file})
        add_custom_command(TARGET TechEngineAPI
                COMMAND ${CMAKE_COMMAND} -E rm -f ${_file}
                COMMAND_EXPAND_LISTS)
    endif()
endforeach()
