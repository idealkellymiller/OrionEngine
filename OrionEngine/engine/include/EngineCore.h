#pragma once

// macro to dllexport or dllimport depending on whether Engine.dll is being built
#ifdef ORN_PLATFORM_WINDOWS
    #ifdef ORN_BUILD_DLL
        #define ORION_API __declspec(dllexport)
    #else
        #define ORION_API __declspec(dllimport)
    #endif
#else
    // #error Orion only supports Windows (for now)!
#endif

#ifdef ORN_ENABLE_ASSERTS
	#define ORN_ASSERT(x, ...) { if(!(x)) { ORN_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define ORN_CORE_ASSERT(x, ...) { if(!(x)) { ORN_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define ORN_ASSERT(x, ...)
	#define ORN_CORE_ASSERT(x, ...)
#endif

