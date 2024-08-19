add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client)

add_custom_target(TechEngineAPI)
add_dependencies(TechEngineAPI TechEngineAPIClient TechEngineAPIServer)

# Set the output directory for the API
set(OUTPUT "${PROJECT_SOURCE_DIR}/bin/engine/api")

# Set target property to store the dll's path for editor and runtime to copy
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "")
    set_target_properties(TechEngineAPI PROPERTIES API_OUTPUT_DIRECTORY "${OUTPUT}")
else ()
    set_target_properties(TechEngineAPI PROPERTIES API_OUTPUT_DIRECTORY "${OUTPUT}")
endif ()

# Empty the include directory
add_custom_command(TARGET TechEngineAPI POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${OUTPUT}/client/include"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT}/client/include"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${OUTPUT}/common/include"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT}/common/include"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${OUTPUT}/server/include"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT}/server/include"
)

# Copy API files to API directory
add_custom_command(TARGET TechEngineAPI POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/engine/API/client/include"
        "${OUTPUT}/client/include"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/engine/API/common/include"
        "${OUTPUT}/common/include"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/engine/API/server/include"
        "${OUTPUT}/server/include"
)