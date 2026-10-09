#pragma once

#if defined(TE_PROFILE_ENABLED)

#include <tracy/Tracy.hpp>

// TE_PROFILER_SCOPE and TE_PROFILER_FUNCTION each declare a fixed-name RAII object,
// so two of them in the same scope is a redeclaration, and only the profile presets compile
// it, so CI stays green while `windows-profile` breaks. Open a nested block for the second.
#define TE_PROFILER_SCOPE(name) ZoneScopedN(name)
#define TE_PROFILER_FUNCTION() ZoneScoped
#define TE_PROFILER_FRAME() FrameMark
#define TE_PROFILER_FRAME_NAMED(name) FrameMarkNamed(name)
// The secure forms check the profiler is up before recording. The global operator new
// replacement runs during CRT static init, which can precede Tracy's own construction.
#define TE_PROFILER_ALLOC(pointer, size) TracyAlloc(pointer, size)
#define TE_PROFILER_FREE(pointer) TracyFree(pointer)
// Tracy keys a memory pool by the name's address, not its text. Pass one named constant to
// every call for a pool; a literal at each call site can split the pool in two.
#define TE_PROFILER_ALLOC_NAMED(pointer, size, name) TracyAllocN(pointer, size, name)
#define TE_PROFILER_FREE_NAMED(pointer, name) TracyFreeN(pointer, name)
#define TE_PROFILER_THREAD_NAME(name) ::tracy::SetThreadName(name)

#else

#define TE_PROFILER_SCOPE(name) ((void)0)
#define TE_PROFILER_FUNCTION() ((void)0)
#define TE_PROFILER_FRAME() ((void)0)
#define TE_PROFILER_FRAME_NAMED(name) ((void)0)
#define TE_PROFILER_ALLOC(pointer, size) ((void)0)
#define TE_PROFILER_FREE(pointer) ((void)0)
// The name is referenced but never evaluated, so a pool constant that only these macros use is
// not an unused variable when profiling is off.
#define TE_PROFILER_ALLOC_NAMED(pointer, size, name) ((void)sizeof(name))
#define TE_PROFILER_FREE_NAMED(pointer, name) ((void)sizeof(name))
#define TE_PROFILER_THREAD_NAME(name) ((void)(name))

#endif
