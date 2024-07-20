# Extract common configurations to a function
function(configure_tech_engine_core target_name)
    file(GLOB_RECURSE ${target_name}_Sources CONFIGURE_DEPENDS "src/*.cpp")
    file(GLOB_RECURSE ${target_name}_Headers "src/*.hpp")

    foreach (_headerFile ${${target_name}_Headers})
        get_filename_component(_dir ${_headerFile} PATH)
        list(APPEND ${target_name}_Include_Dirs ${_dir})
    endforeach ()
    list(REMOVE_DUPLICATES ${target_name}_Include_Dirs)

    add_library(${target_name} SHARED ${${target_name}_Sources} ${${target_name}_Headers})
    target_compile_definitions(${target_name} PUBLIC GLM_ENABLE_EXPERIMENTAL)
    target_compile_definitions(${target_name} PRIVATE TECH_ENGINE_CORE_EXPORTS)
    # Setup target's output directory
    if (WIN32) # WINDOWS
        set(TECH_ENGINE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/TechEngineCore/${target_name}")
        set_target_properties(${target_name} PROPERTIES
                ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_OUTPUT_DIRECTORY}/debug"
                LIBRARY_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_OUTPUT_DIRECTORY}/debug"
                RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TECH_ENGINE_OUTPUT_DIRECTORY}/debug"
                ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_OUTPUT_DIRECTORY}/release"
                LIBRARY_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_OUTPUT_DIRECTORY}/release"
                RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TECH_ENGINE_OUTPUT_DIRECTORY}/release"
        )
    endif ()

    target_include_directories(${target_name} PUBLIC "src")
    target_link_libraries(${target_name} PUBLIC spdlog::spdlog yaml-cpp::yaml-cpp GameNetworkingSockets::static)
    target_compile_definitions(${target_name} PRIVATE
            $<$<CONFIG:Debug>:TE_DEBUG>
            $<$<CONFIG:RelWithDebInfo>:TE_DEBUG>
            $<$<CONFIG:Release>:TE_RELEASEDEBUG>
    )

    # Copy API include files to project template
    set(TECH_ENGINE_API_DIRECTORY "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Common/TechEngineAPI/include/TechEngineCore")
    set(TECH_ENGINE_API_FILES
            "${PROJECT_SOURCE_DIR}/TechEngineCore/src/core/Logger.hpp"
            "${PROJECT_SOURCE_DIR}/TechEngineCore/src/core/Timer.hpp"
            "${PROJECT_SOURCE_DIR}/TechEngineCore/src/core/CoreExportDll.hpp"
            "${PROJECT_SOURCE_DIR}/TechEngineCore/src/system/System.hpp"
            "${PROJECT_SOURCE_DIR}/TechEngineCore/src/events"
            "${PROJECT_SOURCE_DIR}/TechEngineCore/src/components"
            "${PROJECT_SOURCE_DIR}/TechEngineCore/src/physics"
            "${PROJECT_SOURCE_DIR}/TechEngineCore/src/mesh"
            "${PROJECT_SOURCE_DIR}/TechEngineCore/src/script"
            "${PROJECT_SOURCE_DIR}/TechEngineCore/src/material/Material.hpp"
            "${PROJECT_SOURCE_DIR}/TechEngineCore/src/texture"
            "${PROJECT_SOURCE_DIR}/TechEngineCore/src/serialization"
            "${PROJECT_SOURCE_DIR}/TechEngineCore/src/network/NetworkObjectsRegistry.hpp"
            "${PROJECT_SOURCE_DIR}/TechEngineCore/src/scriptingAPI"
    )

    # Clear API files
    file(REMOVE_RECURSE "${TECH_ENGINE_API_DIRECTORY}")

    # Copy API files to API directory
    foreach (entry ${TECH_ENGINE_API_FILES})
        if (IS_DIRECTORY ${entry})
            get_filename_component(folder_name ${entry} NAME)
            add_custom_command(TARGET ${target_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E rm -rf "${TECH_ENGINE_API_DIRECTORY}/${folder_name}"
                    COMMAND_EXPAND_LISTS)
            add_custom_command(TARGET ${target_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory "${entry}" "${TECH_ENGINE_API_DIRECTORY}/${folder_name}"
                    COMMAND_EXPAND_LISTS)
            add_custom_command(TARGET ${target_name} POST_BUILD
                    COMMAND powershell -Command "Get-ChildItem -Path ${TECH_ENGINE_API_DIRECTORY}/${folder_name} -Filter *.cpp -Recurse | Remove-Item -Force"
                    COMMAND_EXPAND_LISTS)
        else ()
            get_filename_component(folder_name ${entry} DIRECTORY)
            get_filename_component(folder_name ${folder_name} NAME)
            if (NOT EXISTS "${TECH_ENGINE_API_DIRECTORY}/${folder_name}")
                file(MAKE_DIRECTORY "${TECH_ENGINE_API_DIRECTORY}/${folder_name}")
            endif ()
            add_custom_command(TARGET ${target_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy "${entry}" "${TECH_ENGINE_API_DIRECTORY}/${folder_name}"
                    COMMAND_EXPAND_LISTS)
        endif ()
    endforeach ()

    # Copy .lib files to project template
    if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR NOT CMAKE_BUILD_TYPE)
        add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Common/TechEngineAPI/lib/debug")
        add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${TECH_ENGINE_OUTPUT_DIRECTORY}/debug/${target_name}.lib"
                "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Common/TechEngineAPI/lib/debug")
        add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${TECH_ENGINE_OUTPUT_DIRECTORY}/debug/${target_name}.pdb"
                "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Common/TechEngineAPI/lib/debug")
    elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
        if (NOT EXISTS "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Common/TechEngineAPI/lib/release")
            file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Common/TechEngineAPI/lib/release")
        endif ()
        add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${TECH_ENGINE_OUTPUT_DIRECTORY}/release/${target_name}.lib"
                "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/Common/TechEngineAPI/lib/release")
    endif ()
endfunction()

# Create TechEngineCoreClient and TechEngineCoreServer libraries
find_package(spdlog CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

# Configure TechEngineCoreClient
configure_tech_engine_core(TechEngineCoreClient)

# Configure TechEngineCoreServer
configure_tech_engine_core(TechEngineCoreServer)

#Configure TechEngineCoreEditor
configure_tech_engine_core(TechEngineCoreEditor)

