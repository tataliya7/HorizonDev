#pragma once

#if !_WIN64
    #error "Platform is not supported"
#endif

#if defined(_MSC_VER)
    #define HORIZON_DISABLE_WARNINGS __pragma(warning(push, 0))
    #define HORIZON_ENABLE_WARNINGS __pragma(warning(pop))
#else
    #define HORIZON_DISABLE_WARNINGS
    #define HORIZON_ENABLE_WARNINGS
#endif

#if defined(_MSC_VER)
    #define FORCEINLINE __forceinline
#else
    #define FORCEINLINE inline
#endif
