#pragma once

#ifndef HE_BUILD_DEBUG
    #if defined(_DEBUG) || defined(DEBUG)
        #define HE_BUILD_DEBUG 1
    #else
        #define HE_BUILD_DEBUG 0
    #endif
#endif

#define HE_MAKE_VERSION(major, minor, patch) ((((uint32)(major)) << 22) | (((uint32)(minor)) << 12) | ((uint32)(patch)))
#define HE_VERSION_MAJOR(version) ((uint32)(version) >> 22)
#define HE_VERSION_MINOR(version) (((uint32)(version) >> 12) & 0x3ff)
#define HE_VERSION_PATCH(version) ((uint32)(version) & 0xfff)
#define HE_VERSION_1_0_0 HE_MAKE_VERSION(1, 0, 0)

#define HE_ENGINE_NAME "Horizon Engine"
#define HE_ENGINE_VERSION HE_VERSION_1_0_0

#if defined(_MSC_VER)
#define HE_DISABLE_WARNINGS __pragma(warning(push, 0))
#define HE_ENABLE_WARNINGS __pragma(warning(pop))
#endif

#if defined(_MSC_VER)
#define HE_DLL_EXPORT __declspec(dllexport)
#else
#define HE_DLL_EXPORT __attribute__((visibility("default")))
#endif

#define HE_ENABLE_ASSERTION

#define HE_TEXT(quote) L##quote

#define HE_DEFAULT_ALIGNMENT uint64(8)

#define HE_CORE_API

#define STATIC_ASSERT(expression) static_assert(expression)

#if defined(HE_ENABLE_ASSERTION)
    #define ASSERT(expression) assert(expression)
#else
    #define ASSERT(expression)
#endif

#if !HE_PLATFORM_WINDOWS
#error "Platform is not supported"
#endif

#if !defined(NOMINMAX) && defined(HE_PLATFORM_WINDOWS)
#define NOMINMAX
#endif

#if defined(_MSC_VER)
#define FORCEINLINE __forceinline
#endif

#define ARRAY_SIZE(a) ((int)(sizeof(a) / sizeof(*(a))))

#define HE_ENUM_CLASS_OPERATORS(EnumClass) inline constexpr EnumClass& operator|=(EnumClass& lhs, EnumClass rhs)   { return lhs = (EnumClass)((__underlying_type(EnumClass))lhs | (__underlying_type(EnumClass))rhs); } \
                                           inline constexpr EnumClass& operator&=(EnumClass& lhs, EnumClass rhs)   { return lhs = (EnumClass)((__underlying_type(EnumClass))lhs & (__underlying_type(EnumClass))rhs); } \
                                           inline constexpr EnumClass& operator^=(EnumClass& lhs, EnumClass rhs)   { return lhs = (EnumClass)((__underlying_type(EnumClass))lhs ^ (__underlying_type(EnumClass))rhs); } \
                                           inline constexpr EnumClass  operator| (EnumClass  lhs, EnumClass rhs)   { return (EnumClass)((__underlying_type(EnumClass))lhs | (__underlying_type(EnumClass))rhs); } \
                                           inline constexpr EnumClass  operator& (EnumClass  lhs, EnumClass rhs)   { return (EnumClass)((__underlying_type(EnumClass))lhs & (__underlying_type(EnumClass))rhs); } \
                                           inline constexpr EnumClass  operator^ (EnumClass  lhs, EnumClass rhs)   { return (EnumClass)((__underlying_type(EnumClass))lhs ^ (__underlying_type(EnumClass))rhs); } \
                                           inline constexpr bool       operator! (EnumClass  e)                    { return !(__underlying_type(EnumClass))e; } \
                                           inline constexpr EnumClass  operator~ (EnumClass  e)                    { return (EnumClass)~(__underlying_type(EnumClass))e; } \
                                           inline constexpr bool       HAS_ANY_FLAGS(EnumClass e, EnumClass flags) { return (e & flags) != (EnumClass)0; }

#define HE_BIND_FUNCTION(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args) > (args)...); }

#define ABORT() abort()

#define HE_DECLARE_HANDLE(object) typedef struct object* object##Handle;

#define HE_TEXT(quote) L##quote

#define HE_ARENA_ALLOC(arena, size)                                          (::HE::ArenaRealloc(arena, nullptr, 0,       size,    HE_DEFAULT_ALIGNMENT, __FILE__, __LINE__))
#define HE_ARENA_FREE(arena, ptr, size)                                      (::HE::ArenaRealloc(arena, ptr,     size,    0,       HE_DEFAULT_ALIGNMENT, __FILE__, __LINE__))
#define HE_ARENA_REALLOC(arena, ptr, oldSize, newSize)                       (::HE::ArenaRealloc(arena, ptr,     oldSize, newSize, HE_DEFAULT_ALIGNMENT, __FILE__, __LINE__))
#define HE_ARENA_ALIGNED_ALLOC(arena, size, alignment)                       (::HE::ArenaRealloc(arena, nullptr, 0,       size,    alignment,            __FILE__, __LINE__))
#define HE_ARENA_ALIGNED_FREE(arena, ptr, size, alignment)                   (::HE::ArenaRealloc(arena, ptr,     size,    0,       alignment,            __FILE__, __LINE__))
#define HE_ARENA_ALIGNED_REALLOC(arena, ptr, oldSize, newSize, alignment)    (::HE::ArenaRealloc(arena, ptr,     oldSize, newSize, alignment,            __FILE__, __LINE__))

#define MAX_NUM_BONES_PER_VERTEX 4

#define HE_DEPRECATED
