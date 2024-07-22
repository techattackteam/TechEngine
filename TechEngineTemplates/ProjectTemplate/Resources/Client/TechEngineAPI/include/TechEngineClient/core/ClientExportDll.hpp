#pragma once

#ifdef TECH_ENGINE_CLIENT_EXPORTS
#define CLIENT_DLL __declspec(dllexport)
#else
#define CLIENT_DLL __declspec(dllimport)
#endif
