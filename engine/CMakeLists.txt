# The 5 static libs, in dependency order (ADR-006 §1): base -> platform -> core,
# client -> {core, platform, base}, app -> core.
add_subdirectory(base)
add_subdirectory(platform)
add_subdirectory(core)
add_subdirectory(client)
add_subdirectory(app)
