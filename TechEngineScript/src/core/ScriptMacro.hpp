#ifdef SCRIPT_BUILD_DLL
#define Script_DLL __declspec(dllexport)
#else
#define Script_DLL __declspec(dllimport)
#endif
