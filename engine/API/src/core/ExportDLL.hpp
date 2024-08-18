#pragma once

#ifdef TECH_ENGINE_CLIENT_API_EXPORT
#define API_DLL __declspec(dllexport)
#else
#define API_DLL __declspec(dllimport)
#endif
