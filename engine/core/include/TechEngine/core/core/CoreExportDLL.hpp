#pragma once

#ifdef TECH_ENGINE_CORE_EXPORT
#define CORE_DLL __declspec(dllexport)
#else
#define CORE_DLL __declspec(dllimport)
#endif
