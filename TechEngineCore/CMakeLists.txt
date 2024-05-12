file(GLOB_RECURSE TechEngineCore_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineCore_Headers "src/*.hpp")

find_package(spdlog CONFIG REQUIRED)
find_package(unofficial-omniverse-physx-sdk CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(GameNetworkingSockets CONFIG REQUIRED)

foreach (_headerFile ${TechEngineCore_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineCore_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineCore_Include_Dirs)

add_library(TechEngineCore SHARED ${TechEngineCore_Sources} ${TechEngineCore_Headers})
target_compile_definitions(TechEngineCore PUBLIC TECH_ENGINE_API_EXPORTS)
target_compile_definitions(TechEngineCore PUBLIC GLM_ENABLE_EXPERIMENTAL)

#Setup target's output directory
if (WIN32) # WINDOWS
    set(TECH_ENGINE_CORE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/TechEngineCore")
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


# Optional: import the defined target to copy over the GPU acceleration libraries (3rd party provided by NVIDIA)
if (TARGET unofficial::omniverse-physx-sdk::gpu-library)
    if (UNIX)
        # Add rpath setting to find .so libraries on unix based systems
        set_target_properties(TechEngineCore PROPERTIES
                BUILD_WITH_INSTALL_RPATH TRUE
                INSTALL_RPATH "$ORIGIN"
        )
    endif ()
    add_custom_command(TARGET TechEngineCore POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:unofficial::omniverse-physx-sdk::gpu-library>
            $<TARGET_FILE_DIR:TechEngineCore>)
    if (WIN32)
        add_custom_command(TARGET TechEngineCore POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:unofficial::omniverse-physx-sdk::gpu-device-library>
                $<TARGET_FILE_DIR:TechEngineCore>)
    endif ()
else ()
    message(WARNING "GPU acceleration library target not defined - GPU acceleration will NOT be available!")
endif ()

target_include_directories(TechEngineCore PUBLIC "src")
target_link_libraries(TechEngineCore PUBLIC spdlog::spdlog unofficial::omniverse-physx-sdk::sdk yaml-cpp::yaml-cpp GameNetworkingSockets::shared)
target_compile_definitions(TechEngineCore PUBLIC
        $<$<CONFIG:Debug>:TE_DEBUG>
        $<$<CONFIG:Release>:TE_RELEASEDEBUG>
        $<$<CONFIG:RelWithDebInfo>:TE_DEBUG>
)

#Copy API include files to project template
set(TECH_ENGINE_CORE_API_DIRECTORY "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Common/TechEngineAPI/include/TechEngineCore")
set(TECH_ENGINE_CORE_API_FILES
        "${PROJECT_SOURCE_DIR}/TechEngineCore/src/core/Logger.hpp"
        "${PROJECT_SOURCE_DIR}/TechEngineCore/src/core/Timer.hpp"
        "${PROJECT_SOURCE_DIR}/TechEngineCore/src/events"
        "${PROJECT_SOURCE_DIR}/TechEngineCore/src/eventSystem"
        "${PROJECT_SOURCE_DIR}/TechEngineCore/src/components"
        "${PROJECT_SOURCE_DIR}/TechEngineCore/src/physics"
        "${PROJECT_SOURCE_DIR}/TechEngineCore/src/mesh"
        "${PROJECT_SOURCE_DIR}/TechEngineCore/src/script"
        #"${PROJECT_SOURCE_DIR}/TechEngineCore/src/scriptingAPI"
        "${PROJECT_SOURCE_DIR}/TechEngineCore/src/material/Material.hpp"
        "${PROJECT_SOURCE_DIR}/TechEngineCore/src/texture"
        "${PROJECT_SOURCE_DIR}/TechEngineCore/src/serialization"
)

#Clear TechEngineClientAPI files
file(REMOVE_RECURSE "${TECH_ENGINE_CORE_API_DIRECTORY}")

#Copy API files to TechEngineCore API directory
foreach (entry ${TECH_ENGINE_CORE_API_FILES})
    if (IS_DIRECTORY ${entry})
        get_filename_component(folder_name ${entry} NAME)
        add_custom_command(TARGET TechEngineCore POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E rm -rf "${TECH_ENGINE_CORE_API_DIRECTORY}/${folder_name}"
                COMMAND_EXPAND_LISTS)
        add_custom_command(TARGET TechEngineCore POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${entry}" "${TECH_ENGINE_CORE_API_DIRECTORY}/${folder_name}"
                COMMAND_EXPAND_LISTS)
        add_custom_command(TARGET TechEngineCore POST_BUILD
                COMMAND powershell -Command "Get-ChildItem -Path ${TECH_ENGINE_CORE_API_DIRECTORY}/${folder_name} -Filter *.cpp -Recurse | Remove-Item -Force"
                COMMAND_EXPAND_LISTS)
    else ()
        get_filename_component(folder_name ${entry} DIRECTORY)
        get_filename_component(folder_name ${folder_name} NAME)
        if (NOT EXISTS "${TECH_ENGINE_CORE_API_DIRECTORY}/${folder_name}")
            file(MAKE_DIRECTORY "${TECH_ENGINE_CORE_API_DIRECTORY}/${folder_name}")
        endif ()
        add_custom_command(TARGET TechEngineCore PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${entry}" "${TECH_ENGINE_CORE_API_DIRECTORY}/${folder_name}"
                COMMAND_EXPAND_LISTS)
    endif ()
endforeach ()

#Copy .lib files to project template
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (NOT EXISTS "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Common/TechEngineAPI/lib/debug")
        file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Common/TechEngineAPI/lib/debug")
    endif ()
    add_custom_command(TARGET TechEngineCore POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "${TECH_ENGINE_CORE_OUTPUT_DIRECTORY}/debug/TechEngineCore.lib"
            "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Common/TechEngineAPI/lib/debug")
    add_custom_command(TARGET TechEngineCore POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "${TECH_ENGINE_CORE_OUTPUT_DIRECTORY}/debug/TechEngineCore.pdb"
            "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Common/TechEngineAPI/lib/debug")
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    if (NOT EXISTS "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Common/TechEngineAPI/lib/release")
        file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Common/TechEngineAPI/lib/release")
    endif ()
    add_custom_command(TARGET TechEngineCore POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "${TECH_ENGINE_CORE_OUTPUT_DIRECTORY}/release/TechEngineCore.lib"
            "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Common/TechEngineAPI/lib/release")
endif ()