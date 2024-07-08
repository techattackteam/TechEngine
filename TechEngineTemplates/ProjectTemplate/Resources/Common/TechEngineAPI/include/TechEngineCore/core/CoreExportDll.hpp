#pragma once

#ifdef TECH_ENGINE_CORE_EXPORTS
#define CORE_DLL __declspec(dllexport)
#else
#define CORE_DLL __declspec(dllimport)
#endif

