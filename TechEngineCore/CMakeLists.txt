file(GLOB_RECURSE TechEngineCore_Sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TechEngineCore_Headers "src/*.hpp")

find_package(glm CONFIG REQUIRED)
find_package(Boost COMPONENTS serialization)

add_definitions(-DENGINE_BUILD_DLL)

foreach (_headerFile ${TechEngineCore_Headers})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND TechEngineCore_Include_Dirs ${_dir})
endforeach ()
list(REMOVE_DUPLICATES TechEngineCore_Include_Dirs)

add_library(TechEngineCore SHARED ${TechEngineCore_Sources} ${TechEngineCore_Headers})
target_include_directories(TechEngineCore PUBLIC "src")
target_link_libraries(TechEngineCore Boost::boost Boost::serialization)


set_target_properties(TechEngineCore PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TechEngineCore/archive"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TechEngineCore/libs"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TechEngineCore/runtime"
        )

add_custom_command(TARGET TechEngineCore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_RUNTIME_DLLS:TechEngineEditor> $<TARGET_FILE_DIR:TechEngineEditor>
        COMMAND_EXPAND_LISTS)

add_custom_command(TARGET TechEngineCore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE_DIR:Game> $<TARGET_FILE_DIR:TechEngineEditor>/runtime
        COMMAND_EXPAND_LISTS)