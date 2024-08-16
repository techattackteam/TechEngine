#pragma once

#ifdef TECH_ENGINE_SERVER_EXPORT
#define SERVER_DLL __declspec(dllexport)
#else
#define SERVER_DLL __declspec(dllimport)
#endif
