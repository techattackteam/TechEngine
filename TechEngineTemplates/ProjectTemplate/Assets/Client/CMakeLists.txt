

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../common/dependencies/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "Vcpkg target triplet")
project(UserProject)

set(CMAKE_CXX_STANDARD 20)
set(TechEngineCoreLIB   "C:/dev/TechEngine/bin/TechEngineEditor/debug/DefaultProject/Resources/Common/TechEngineAPI/lib/debug/TechEngineCoreClient.lib" CACHE STRING "File path for the TechEngineCore lib")
set(TechEngineClientLIB "C:/dev/TechEngine/bin/TechEngineEditor/debug/DefaultProject/Resources/Client/TechEngineAPI/lib/debug/TechEngineClient.lib"     CACHE STRING "File path for the TechEngineClient lib")

find_package(spdlog CONFIG REQUIRED)

file(GLOB_RECURSE ClientScripts_Client_Sources CONFIGURE_DEPENDS "../../../Assets/Client/*.cpp")
file(GLOB_RECURSE ClientScripts_Client_Headers "../../../Assets/Client/*.hpp")
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

add_library(ClientScripts SHARED ${ClientScripts_Client_Sources} ${ClientScripts_Client_Headers} ${ClientScripts_Common_Sources} ${ClientScripts_Common_Headers})

if (WIN32) # WINDOWS
    set(TECH_ENGINE_API_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../scripts/build")
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
        "../../Resources/Common/TechEngineAPI/include/TechEngineCore"
        "../../Resources/Common/TechEngineAPI"
        "../../Resources/Client/TechEngineAPI/include/TechEngineClient"
        "../../Resources/ClientTechEngineAPI/include")

target_link_libraries(ClientScripts PUBLIC ${TechEngineCoreLIB} ${TechEngineClientLIB} spdlog::spdlog)
