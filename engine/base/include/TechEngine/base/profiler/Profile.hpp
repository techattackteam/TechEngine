#pragma once

#if defined(TE_PROFILE_ENABLED)

#include <tracy/Tracy.hpp>

// TE_PROFILER_SCOPE and TE_PROFILER_FUNCTION each declare a fixed-name RAII object,
// so two of them in the same scope is a redeclaration — and only the profile presets compile
// it, so CI stays green while `windows-profile` breaks. Open a nested block for the second.
#define TE_PROFILER_SCOPE(name) ZoneScopedN(name)
#define TE_PROFILER_FUNCTION() ZoneScoped
#define TE_PROFILER_FRAME() FrameMark

#else

#define TE_PROFILER_SCOPE(name) ((void)0)
#define TE_PROFILER_FUNCTION() ((void)0)
#define TE_PROFILER_FRAME() ((void)0)

#endif
