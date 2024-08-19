file(GLOB_RECURSE ClientScripts_Client_Sources "*.cpp")
file(GLOB_RECURSE ClientScripts_Client_Headers "*.hpp")
foreach (_headerFile ${TechEngineAPI_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineAPI_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineAPIInclude_Dirs)

file(GLOB_RECURSE ClientScripts_Common_Sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE ClientScripts_Common_Headers "*.hpp")
foreach (_headerFile ${TechEngineAPI_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineAPI_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineAPIInclude_Dirs)

find_package(spdlog REQUIRED)

add_library(ClientScripts SHARED ${ClientScripts_Client_Sources} ${ClientScripts_Client_Headers} ${ClientScripts_Common_Sources} ${ClientScripts_Common_Headers})
target_compile_definitions(ClientScripts PRIVATE GLM_ENABLE_EXPERIMENTAL)
target_compile_definitions(ClientScripts PRIVATE TECH_ENGINE_CLIENT_SCRIPTS_EXPORT)

target_link_libraries(ClientScripts PUBLIC spdlog)

if (WIN32) # WINDOWS
    set(TECH_ENGINE_API_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../resources/client/scripts/build")
    set_target_properties(ClientScripts PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/debug"
    )
    set_target_properties(ClientScripts PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/release"
    )
    set_target_properties(ClientScripts PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/releaseWithDebug"
            LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/releaseWithDebug"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/releaseWithDebug"
    )
endif ()
target_include_directories(ClientScripts PUBLIC
        "../../resources/common/TechEngineAPI/include")

# Set variable for TechEngineAPILib
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(TECH_ENGINE_API_LIB "${CMAKE_SOURCE_DIR}/../resources/common/TechEngineAPI/lib/debug/TechEngineAPI.lib")
else ()
    set(TECH_ENGINE_API_LIB "${CMAKE_SOURCE_DIR}/../resources/common/TechEngineAPI/lib/release/TechEngineAPI.lib")
endif ()
target_link_libraries(ClientScripts PUBLIC ${TECH_ENGINE_API_LIB})