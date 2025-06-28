# findCmake/FindRmlUI.cmake
#
# Finds a pre-built RmlUi library installation and its bundled Freetype dependency.
#
# This module will use the RMLUI_ROOT_DIR cache variable as a hint to the root
# of the RmlUi installation.
#
# It defines the following:
#  RmlUI_FOUND        - True if RmlUi and its dependencies are found.
#  RmlUi::Core        - The imported target for the RmlUi core library.
#  RmlUi::Freetype    - The imported target for the Freetype library bundled with RmlUi.

# 1. Find the include directory
find_path(RmlUI_INCLUDE_DIR
        NAMES RmlUi/Core.h
        PATHS ${CMAKE_SOURCE_DIR}/libs/rmlUI/include # Use the user-provided hint first
        DOC "RmlUi include directory"
        PATH_SUFFIXES RmlUi
)

# 2. Find the RmlUi library files
# The pre-built binaries use the name 'RmlCore.lib', not 'rmlui.lib'
find_library(RmlUI_LIBRARY_RELEASE
        NAMES rmlui
        PATHS
        ${CMAKE_SOURCE_DIR}/libs/rmlUI/bin/Release
        DOC "RmlUi Release library"
)

# The pre-built release zip often does NOT contain a debug library. This search will likely fail,
# which is why we won't make it a required variable later.
find_library(RmlUI_LIBRARY_DEBUG
        NAMES rmlui # A common convention for debug libraries
        PATHS
        ${CMAKE_SOURCE_DIR}/libs/rmlUI/bin/Debug
        DOC "RmlUi Debug library"
)

# 3. Find the Freetype library that is BUNDLED with RmlUi
# This is crucial for ensuring compatibility.
find_path(RmlUI_FREETYPE_INCLUDE_DIR
        NAMES ft2build.h
        HINTS
        ${CMAKE_SOURCE_DIR}/libs/rmlUI/dependencies/include # Use the user-provided hint first
        DOC "RmlUi include directory"
)

find_library(RmlUI_FREETYPE_LIBRARY
        NAMES freetype
        HINTS
        ${CMAKE_SOURCE_DIR}/libs/rmlUI/dependencies/lib
        DOC "Freetype library bundled with RmlUi"
)

# 4. Handle standard arguments and set RmlUI_FOUND
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RmlUI
        FOUND_VAR RmlUI_FOUND
        REQUIRED_VARS RmlUI_INCLUDE_DIR RmlUI_LIBRARY_DEBUG RmlUI_LIBRARY_RELEASE RmlUI_FREETYPE_LIBRARY RmlUI_FREETYPE_INCLUDE_DIR
)

# 5. Create imported targets if the library was found
if (RmlUI_FOUND)
    # --- Create the target for the BUNDLED Freetype ---
    # We check if it already exists to be safe.
    if (NOT TARGET RmlUi::Freetype)
        add_library(RmlUi::Freetype STATIC IMPORTED)
        set_target_properties(RmlUi::Freetype PROPERTIES
                IMPORTED_LOCATION "${RmlUI_FREETYPE_LIBRARY}"
        )
    endif ()

    # --- Create the main RmlUi::Core target ---
    if (NOT TARGET RmlUi::Core)
        add_library(RmlUi::Core STATIC IMPORTED)
        set_target_properties(RmlUi::Core PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${RmlUI_INCLUDE_DIR}"
        )
        # Link RmlUi against its bundled Freetype dependency.
        # This is the magic that makes it all work.
        target_link_libraries(RmlUi::Core INTERFACE RmlUi::Freetype)
    endif ()

    # Set the actual library locations for Release and (optional) Debug
    set_target_properties(RmlUi::Core PROPERTIES
            IMPORTED_LOCATION_RELEASE "${RmlUI_LIBRARY_RELEASE}"
    )

    # If a debug library was found, use it for the Debug configuration.
    # Otherwise, fall back to using the Release library for debugging too.
    if (RmlUI_LIBRARY_DEBUG)
        set_target_properties(RmlUi::Core PROPERTIES
                IMPORTED_LOCATION_DEBUG "${RmlUI_LIBRARY_DEBUG}"
        )
    else ()
        # No debug lib found, so we map Debug to the Release lib.
        set_target_properties(RmlUi::Core PROPERTIES
                IMPORTED_LOCATION_DEBUG "${RmlUI_LIBRARY_RELEASE}"
        )
    endif ()

    # CRITICAL FIX: For all other release-type configurations, also map them to the Release library.
    set_target_properties(RmlUi::Core PROPERTIES
            IMPORTED_LOCATION_RELWITHDEBINFO "${RmlUI_LIBRARY_RELEASE}"
            IMPORTED_LOCATION_MINSIZEREL "${RmlUI_LIBRARY_RELEASE}"
    )

    message(STATUS "Found RmlUi and bundled Freetype")
else ()
    message(WARNING "Could not find RmlUi. Please set RMLUI_ROOT_DIR to the root of your RmlUi SDK.")
endif ()

# Hide internal variables from the CMake GUI
mark_as_advanced(RmlUI_INCLUDE_DIR RmlUI_LIBRARY_RELEASE RmlUI_LIBRARY_DEBUG RmlUI_FREETYPE_LIBRARY)