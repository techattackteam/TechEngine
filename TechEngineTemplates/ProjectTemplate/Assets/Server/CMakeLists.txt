set(TechEngineCoreLIB "TechEngineCoreServerLIB-placeholder" CACHE STRING "TechEngineCoreServer.lib")
set(TechEngineServerLIB "TechEngineServerLIB-placeholder" CACHE STRING "TechEngineServer.lib")
find_package(spdlog CONFIG REQUIRED)


file(GLOB_RECURSE ServerScripts_Server_Sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE ServerScripts_Server_Headers "*.hpp")
foreach (_headerFile ${TechEngineAPI_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineAPI_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineAPIInclude_Dirs)

file(GLOB_RECURSE ServerScripts_Common_Sources CONFIGURE_DEPENDS "../../../Assets/Common/*.cpp")
file(GLOB_RECURSE ServerScripts_Common_Headers "../../../Assets/Common/*.hpp")
foreach (_headerFile ${TechEngineAPI_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineAPI_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineAPIInclude_Dirs)


add_library(ServerScripts SHARED ${ServerScripts_Server_Sources} ${ServerScripts_Server_Headers} ${ServerScripts_Common_Sources} ${ServerScripts_Common_Headers})
target_include_directories(ServerScripts PRIVATE "../../Common/dependencies/GameNetworkingSockets")
target_compile_definitions(ServerScripts PRIVATE GLM_ENABLE_EXPERIMENTAL)

if (WIN32) # WINDOWS
    set(TECH_ENGINE_API_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../Resources/Server/scripts/build")
    set_target_properties(ServerScripts PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/debug"
    )
    set_target_properties(ServerScripts PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/release"
    )
    set_target_properties(ServerScripts PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/releaseWithDebug"
            LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/releaseWithDebug"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${TECH_ENGINE_API_OUTPUT_DIRECTORY}/releaseWithDebug"
    )
endif ()
target_include_directories(ServerScripts PUBLIC
        "../../Resources/Common/TechEngineAPI/include/TechEngineCore"
        "../../Resources/Common/TechEngineAPI"
        "../../Resources/Server/TechEngineAPI/include/TechEngineServer"
        "../../Resources/Server/TechEngineAPI/include")
target_link_libraries(ServerScripts ${TechEngineCoreLIB} ${TechEngineServerLIB} GameNetworkingSockets::static spdlog::spdlog)
