#pragma once

#ifdef TECH_ENGINE_API_EXPORTS
#define TECHENGINE_API __declspec(dllexport)
#else
#define TECHENGINE_API __declspec(dllimport)
#endif
