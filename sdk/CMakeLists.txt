# te_sdk — the script SDK boundary (ADR-006 §3, ADR-008 §7): a curated INTERFACE
# target over a hand-picked include/ tree, NOT the module include/ dirs. Exposes
# only facade types whose definitions ship here.
add_library(TechEngineSDK INTERFACE)
add_library(TechEngine::sdk ALIAS TechEngineSDK)
target_include_directories(TechEngineSDK INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(TechEngineSDK INTERFACE cxx_std_20)

# te_sdk_smoke — compiles against TechEngine::sdk ALONE. Built as part of ALL, so a
# private-type leak into the SDK breaks the build; also registered as a CTest case.
add_executable(TechEngineSDKSmoke smoke/smoke.cpp)
target_link_libraries(TechEngineSDKSmoke PRIVATE TechEngine::sdk te_warnings)
set_target_properties(TechEngineSDKSmoke PROPERTIES FOLDER "sdk")

if(TE_BUILD_TESTS)
    add_test(NAME sdk-smoke COMMAND TechEngineSDKSmoke)
endif()
