set(CMAKE_CXX_STANDARD 20)
set(TechEngineCoreLIB   "C:/dev/TechEngine/bin/TechEngineEditor/debug/DefaultProject/Resources/Common/TechEngineAPI/lib/debug/TechEngineCoreServer.lib"CACHE STRING "File path for the TechEngineCore lib")
set(TechEngineServerLIB "C:/dev/TechEngine/bin/TechEngineEditor/debug/DefaultProject/Resources/Server/TechEngineAPI/lib/debug/TechEngineServer.lib"    CACHE STRING "File path for the TechEngineClient lib")

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

if (WIN32) # WINDOWS
    set(TECH_ENGINE_API_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../scripts/build")
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
        "../../Common/TechEngineAPI/include/TechEngineCore"
        "../../Common/TechEngineAPI"
        "../TechEngineAPI/include/TechEngineServer"
        "../TechEngineAPI/include"
        "../../Common/dependencies/glm"
        "../../Common/dependencies/yaml-cpp/include")
target_link_libraries(ServerScripts ${TechEngineCoreLIB} ${TechEngineServerLIB} GameNetworkingSockets::static)
