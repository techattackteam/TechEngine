
# include all source files from the src directory
file(GLOB_RECURSE EDITOR_SOURCES "src/*.cpp")
file(GLOB_RECURSE EDITOR_HEADERS "src/*.hpp")

find_package(ImGui REQUIRED)
find_package(ImGuizmo REQUIRED)

# add the executable
add_executable(TechEngineEditor ${EDITOR_SOURCES} ${EDITOR_HEADERS})
target_include_directories(TechEngineEditor PUBLIC src)
target_link_libraries(TechEngineEditor TechEngineApp TechEngineCore TechEngineClient TechEngineServer imgui imguizmo)
target_compile_definitions(TechEngineEditor PUBLIC TECH_ENGINE_EDITOR)
target_compile_definitions(TechEngineEditor PUBLIC TECH_ENGINE_CORE_EXPORT=)

# setup target's output directory
if (WIN32) # WINDOWS
    set_target_properties(TechEngineEditor PROPERTIES TECH_ENGINE_EDITOR_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/runtime/editor/")
    get_target_property(OUTPUT TechEngineEditor TECH_ENGINE_EDITOR_OUTPUT_DIRECTORY)
    set_target_properties(TechEngineEditor PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT}/debug"
    )
    set_target_properties(TechEngineEditor PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT}/release"
    )
endif ()

# Copy the dlls to the output directory
if (WIN32) # WINDOWS
    get_target_property(ClientDLL TechEngineClient CLIENT_DLL)
    get_target_property(ServerDLL TechEngineServer SERVER_DLL)
    get_target_property(Client_API_DLL TechEngineAPIClient CLIENT_API_DLL)
    get_target_property(SERVER_API_DLL TechEngineAPIServer SERVER_API_DLL)
    add_custom_command(TARGET TechEngineEditor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${ClientDLL} $<TARGET_FILE_DIR:TechEngineEditor>
            COMMAND ${CMAKE_COMMAND} -E copy ${ServerDLL} $<TARGET_FILE_DIR:TechEngineEditor>
            COMMAND ${CMAKE_COMMAND} -E copy ${Client_API_DLL} $<TARGET_FILE_DIR:TechEngineEditor>
            COMMAND ${CMAKE_COMMAND} -E copy ${SERVER_API_DLL} $<TARGET_FILE_DIR:TechEngineEditor>
    )
endif ()

# Copy runtime to editor
if (WIN32)
    add_custom_target(Runtimes ALL DEPENDS TechEngineRuntimeClient TechEngineRuntimeServer
    )

    # Add the runtime dependencies to the editor
    add_dependencies(TechEngineEditor Runtimes)

    get_target_property(ClientRuntime TechEngineRuntimeClient RUNTIME_CLIENT_OUTPUT_DIRECTORY)
    # Add commands to copy the runtime files to the editor directory
    add_custom_command(TARGET TechEngineEditor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E remove_directory $<TARGET_FILE_DIR:TechEngineEditor>/runtimes/client
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${ClientRuntime}
            $<TARGET_FILE_DIR:TechEngineEditor>/runtimes/client
    )

    get_target_property(ServerRuntime TechEngineRuntimeServer RUNTIME_SERVER_OUTPUT_DIRECTORY)
    # Add commands to copy the runtime files to the editor directory
    add_custom_command(TARGET TechEngineEditor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E remove_directory $<TARGET_FILE_DIR:TechEngineEditor>/runtimes/server
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${ServerRuntime}
            $<TARGET_FILE_DIR:TechEngineEditor>/runtimes/server
    )
endif ()

# Create folders
add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory $<TARGET_FILE_DIR:TechEngineEditor>/assets
        COMMAND ${CMAKE_COMMAND} -E remove_directory $<TARGET_FILE_DIR:TechEngineEditor>/cache
        COMMAND ${CMAKE_COMMAND} -E remove_directory $<TARGET_FILE_DIR:TechEngineEditor>/resources
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TechEngineEditor>/assets
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TechEngineEditor>/cache
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TechEngineEditor>/resources

)
add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:TechEngineEditor>/resources/templates
)

# Copy the project template to the resources templates folder
add_custom_command(TARGET TechEngineEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/templates/project
        $<TARGET_FILE_DIR:TechEngineEditor>/resources/templates/project
)

# Copy API folder to project template folder
if (WIN32)
    add_dependencies(TechEngineEditor TechEngineAPI)

    get_target_property(API TechEngineAPI API_OUTPUT_DIRECTORY)

    add_custom_command(TARGET TechEngineEditor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${API}/client
            $<TARGET_FILE_DIR:TechEngineEditor>/resources/templates/project/resources/client/TechEngineAPI
    )
    add_custom_command(TARGET TechEngineEditor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${API}/common
            $<TARGET_FILE_DIR:TechEngineEditor>/resources/templates/project/resources/common/TechEngineAPI
    )
    add_custom_command(TARGET TechEngineEditor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${API}/server
            $<TARGET_FILE_DIR:TechEngineEditor>/resources/templates/project/resources/server/TechEngineAPI
    )
endif ()

