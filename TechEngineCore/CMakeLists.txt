file(GLOB_RECURSE TechEngineCore_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineCore_Headers "src/*.hpp")

find_package(Boost COMPONENTS serialization)
find_package(spdlog CONFIG REQUIRED)
find_package(unofficial-omniverse-physx-sdk CONFIG REQUIRED)

add_definitions(-DENGINE_BUILD_DLL)

foreach (_headerFile ${TechEngineCore_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineCore_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineCore_Include_Dirs)

add_library(TechEngineCore STATIC ${TechEngineCore_Sources} ${TechEngineCore_Headers})


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
if(TARGET unofficial::omniverse-physx-sdk::gpu-library)
    if(UNIX)
        # Add rpath setting to find .so libraries on unix based systems
        set_target_properties(TechEngineCore PROPERTIES
                BUILD_WITH_INSTALL_RPATH TRUE
                INSTALL_RPATH "$ORIGIN"
        )
    endif()
    add_custom_command(TARGET TechEngineCore POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:unofficial::omniverse-physx-sdk::gpu-library>
            $<TARGET_FILE_DIR:TechEngineCore>)
    if(WIN32)
        add_custom_command(TARGET TechEngineCore POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:unofficial::omniverse-physx-sdk::gpu-device-library>
                $<TARGET_FILE_DIR:TechEngineCore>)
    endif()
else()
    message(WARNING "GPU acceleration library target not defined - GPU acceleration will NOT be available!")
endif()

target_include_directories(TechEngineCore PUBLIC "src")
target_link_libraries(TechEngineCore PUBLIC Boost::boost Boost::serialization spdlog::spdlog unofficial::omniverse-physx-sdk::sdk)
