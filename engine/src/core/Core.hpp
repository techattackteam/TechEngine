#pragma once

#ifdef ENGINE_BUILD_DLL
    #define Engine_API __declspec(dllexport)
#else
    #define Engine_API __declspec(dllimport)
#endif