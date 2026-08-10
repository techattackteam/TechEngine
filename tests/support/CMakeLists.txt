# te_test_support — header-only helpers shared by more than one module's suite. Build-only,
# so it keeps the te_* spelling (B4 — Code Conventions → Target naming scheme) and never
# ships. techengine_test() links it into every suite, so a helper lands here the moment a
# second module needs it rather than being pasted.
add_library(te_test_support INTERFACE)
target_include_directories(te_test_support INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(te_test_support INTERFACE cxx_std_20)
