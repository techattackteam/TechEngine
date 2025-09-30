#pragma once

#ifdef TECH_ENGINE_CLIENT_EXPORT
#define CLIENT_DLL __declspec(dllexport)
#else
#define CLIENT_DLL __declspec(dllimport)
#endif
