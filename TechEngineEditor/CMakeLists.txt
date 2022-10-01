file(GLOB_RECURSE TechEngineEditor_Sources "src/*.cpp")
file(GLOB_RECURSE TechEngineEditor_Headers "src/*.hpp")

find_package(imgui CONFIG REQUIRED)

add_executable(TechEngineEditor ${TechEngineEditor_Sources} ${TechEngineEditor_Headers})

target_include_directories(TechEngineEditor PRIVATE "../TechEngineGame/src/external")
target_include_directories(TechEngineEditor PRIVATE "src")
target_link_libraries(TechEngineEditor TechEngineGame imgui::imgui)

