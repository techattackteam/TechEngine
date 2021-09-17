file(GLOB_RECURSE Engine_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE Engine_Headers "src/*.hpp")

find_package(glm REQUIRED)
find_package(Boost COMPONENTS program_options REQUIRED)

add_subdirectory(lib/imgui build)

foreach (_headerFile ${Engine_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND Engine_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES Engine_Include_Dirs)

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})
include_directories(${Boost_INCLUDE_DIR})

link_libraries(${Boost_LIBRARIES} -lboost_serialization)
add_library(engine SHARED ${Engine_Sources} ${Engine_Headers})
target_link_libraries(engine -lglut -lglfw -lGLEW -lGL -lGLU imgui glm::glm)

add_custom_target(copy_shaders ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/engine/res/ ${CMAKE_CURRENT_BINARY_DIR}/res)