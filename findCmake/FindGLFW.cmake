# findCmake/FindGLFW.cmake

# - Find glfw
# Find the native GLFW includes and library
#
#  GLFW_INCLUDE_DIRS - Where to find GLFW include directories
#  GLFW_LIBRARIES     - List of GLFW libraries (full paths)
#  GLFW_FOUND        - True if GLFW is found.

find_path(GLFW_INCLUDE_DIR
        NAMES GLFW/glfw3.h
        PATHS ${CMAKE_SOURCE_DIR}/libs/GLFW/include
        PATH_SUFFIXES GLFW
)

find_library(GLFW_LIBRARY
        NAMES glfw3dll
        PATHS ${CMAKE_SOURCE_DIR}/libs/GLFW/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(glfw DEFAULT_MSG GLFW_INCLUDE_DIR GLFW_LIBRARY)

mark_as_advanced(GLFW_INCLUDE_DIR GLFW_LIBRARY)

set(GLFW_FOUND TRUE)

# Create a normal library target for GLFW
add_library(glfw STATIC IMPORTED)

# Set the include directories and library for the target
set_target_properties(glfw PROPERTIES
        IMPORTED_LOCATION ${GLFW_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${GLFW_INCLUDE_DIR})