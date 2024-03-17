file(GLOB_RECURSE TechEngineCore_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineCore_Headers "src/*.hpp")

find_package(Boost COMPONENTS serialization)
find_package(spdlog CONFIG REQUIRED)
find_package(unofficial-omniverse-physx-sdk CONFIG REQUIRED)


foreach (_headerFile ${TechEngineCore_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineCore_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineCore_Include_Dirs)

add_library(TechEngineCore SHARED ${TechEngineCore_Sources} ${TechEngineCore_Headers})
add_compile_definitions(TECH_ENGINE_API_EXPORTS)
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
target_link_libraries(TechEngineCore PUBLIC Boost::boost Boost::serialization spdlog::spdlog unofficial::omniverse-physx-sdk::sdk)

#Copy API include files to project template
set(TECH_ENGINE_CLIENT_API_DIRECTORY "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/TechEngineAPI/include/TechEngineCore")
set(TECH_ENGINE_CLIENT_API_FILES
        "${PROJECT_SOURCE_DIR}/TechEngineCore/src/core/Logger.hpp"
        "${PROJECT_SOURCE_DIR}/TechEngineCore/src/core/Timer.hpp"
        "${PROJECT_SOURCE_DIR}/TechEngineCore/src/event/events"
        "${PROJECT_SOURCE_DIR}/TechEngineCore/src/components"
        "${PROJECT_SOURCE_DIR}/TechEngineCore/src/physics"
        "${PROJECT_SOURCE_DIR}/TechEngineCore/src/mesh"
)
foreach (entry ${TECH_ENGINE_CLIENT_API_FILES})
    if (IS_DIRECTORY ${entry})
        get_filename_component(folder_name ${entry} NAME)
        if (NOT EXISTS "${TECH_ENGINE_CLIENT_API_FILES}/${folder_name}")
            file(MAKE_DIRECTORY "${TECH_ENGINE_CLIENT_API_DIRECTORY}/${folder_name}")
        else ()
            add_custom_command(TARGET TechEngineCore POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E rm -rf "${TECH_ENGINE_CLIENT_API_DIRECTORY}/${folder_name}"
                    COMMAND_EXPAND_LISTS)
        endif ()
        add_custom_command(TARGET TechEngineCore POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${entry}" "${TECH_ENGINE_CLIENT_API_DIRECTORY}/${folder_name}"
                COMMAND_EXPAND_LISTS)
        add_custom_command(TARGET TechEngineCore POST_BUILD
                COMMAND powershell -Command "Get-ChildItem -Path ${TECH_ENGINE_CLIENT_API_DIRECTORY}/${folder_name} -Filter *.cpp -Recurse | Remove-Item -Force"
                COMMAND_EXPAND_LISTS)
    else ()
        get_filename_component(folder_name ${entry} DIRECTORY)
        get_filename_component(folder_name ${folder_name} NAME)
        if (NOT EXISTS "${TECH_ENGINE_CLIENT_API_DIRECTORY}/${folder_name}")
            file(MAKE_DIRECTORY "${TECH_ENGINE_CLIENT_API_DIRECTORY}/${folder_name}")
        endif ()
        add_custom_command(TARGET TechEngineCore PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${entry}" "${TECH_ENGINE_CLIENT_API_DIRECTORY}/${folder_name}"
                COMMAND_EXPAND_LISTS)
    endif ()
endforeach ()

#Copy DLLs to project template
if (WIN32)
    set(TECH_ENGINE_CLIENT_DLL_DIRECTORY "${PROJECT_SOURCE_DIR}/TechEngineTemplates/ProjectTemplate/Resources/TechEngineAPI/lib")
    add_custom_command(TARGET TechEngineCore POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:TechEngineCore>
            "${TECH_ENGINE_CLIENT_DLL_DIRECTORY}")
    add_custom_command(TARGET TechEngineCore POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:unofficial::omniverse-physx-sdk::sdk>
            "${TECH_ENGINE_CLIENT_DLL_DIRECTORY}")
    add_custom_command(TARGET TechEngineCore POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:unofficial::omniverse-physx-sdk::gpu-library>
            "${TECH_ENGINE_CLIENT_DLL_DIRECTORY}")
    add_custom_command(TARGET TechEngineCore POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:unofficial::omniverse-physx-sdk::gpu-device-library>
            "${TECH_ENGINE_CLIENT_DLL_DIRECTORY}")
endif ()