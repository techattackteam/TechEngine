# FindCmake/FindAssimp.cmake

# - Find Assimp
# Find the native assimp includes and library
#
#  assimp_INCLUDE_DIRS - Where to find assimp include directories
#  assimp_LIBRARIES     - List of assimp libraries (full paths)
#  assimp_FOUND        - True if assimp is found.

find_path(assimp_INCLUDE_DIR
        NAMES assimp/Importer.hpp
        PATHS ${CMAKE_SOURCE_DIR}/libs/assimp/include
        PATH_SUFFIXES assimp
)

find_library(assimp_LIBRARY
        NAMES assimp-vc143-mt
        PATHS ${CMAKE_SOURCE_DIR}/libs/assimp/lib/Debug
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(assimp DEFAULT_MSG assimp_INCLUDE_DIR assimp_LIBRARY)

mark_as_advanced(assimp_INCLUDE_DIR assimp_LIBRARY)

if (assimp_FOUND)
    # Set the Assimp found flag
    set(assimp_FOUND TRUE)

    # Define necessary macros for static linking
    add_definitions(-DASSIMP_BUILD_NO_EXPORT)
    add_definitions(-DASSIMP_BUILD_NO_OWN_ZLIB)  # If Assimp uses its own zlib, adjust this accordingly

    # Find OpenGL as it is required by Assimp
    find_package(OpenGL REQUIRED)

    # Create an interface target for Assimp
    add_library(assimp STATIC IMPORTED)
    set_target_properties(assimp PROPERTIES
            IMPORTED_LOCATION ${assimp_LIBRARY}
            INTERFACE_INCLUDE_DIRECTORIES ${assimp_INCLUDE_DIR}
    )

    # Assuming Assimp requires zlib and other dependencies; you might need to locate them
    # For example, if zlib is required:
    find_package(ZLIB REQUIRED)
    target_link_libraries(assimp INTERFACE ZLIB::ZLIB OpenGL::GL)  # Link Zlib and OpenGL with Assimp

    # You might need to add other dependencies as well:
    # find_package(IrrXML REQUIRED)  # If Assimp uses IrrXML
    # target_link_libraries(assimp INTERFACE IrrXML::IrrXML)

    # Add additional dependencies for Assimp if needed
    # target_link_libraries(assimp INTERFACE ${ADDITIONAL_LIBRARIES})

else ()
    set(assimp_FOUND FALSE)
endif ()
