#pragma once

#ifdef TECH_ENGINE_SERVER_EXPORTS
#define SERVER_DLL __declspec(dllexport)
#else
#define SERVER_DLL __declspec(dllimport)
#endif
