
# Include all source files from the src directory
file(GLOB APP_SOURCES "src/*.cpp")
file(GLOB APP_HEADERS "src/*.hpp")

add_library(TechEngineApp STATIC ${APP_SOURCES} ${APP_HEADERS})
target_include_directories(TechEngineApp PUBLIC src)

#Setup target's output directory
if (WIN32) # WINDOWS
    set_target_properties(TechEngineApp PROPERTIES TECH_ENGINE_APP_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/engine/app/")
    get_target_property(OUTPUT TechEngineApp TECH_ENGINE_APP_OUTPUT_DIRECTORY)
    set_target_properties(TechEngineApp PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
    )
    set_target_properties(TechEngineApp PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
    )
endif ()
