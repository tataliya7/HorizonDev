#pragma once

#define HORIZON_ENUM_CLASS_OPERATORS(EnumClass) inline constexpr EnumClass& operator|=(EnumClass& lhs, EnumClass rhs)   { return lhs = (EnumClass)((__underlying_type(EnumClass))lhs | (__underlying_type(EnumClass))rhs); } \
                                                inline constexpr EnumClass& operator&=(EnumClass& lhs, EnumClass rhs)   { return lhs = (EnumClass)((__underlying_type(EnumClass))lhs & (__underlying_type(EnumClass))rhs); } \
                                                inline constexpr EnumClass& operator^=(EnumClass& lhs, EnumClass rhs)   { return lhs = (EnumClass)((__underlying_type(EnumClass))lhs ^ (__underlying_type(EnumClass))rhs); } \
                                                inline constexpr EnumClass  operator| (EnumClass  lhs, EnumClass rhs)   { return (EnumClass)((__underlying_type(EnumClass))lhs | (__underlying_type(EnumClass))rhs); } \
                                                inline constexpr EnumClass  operator& (EnumClass  lhs, EnumClass rhs)   { return (EnumClass)((__underlying_type(EnumClass))lhs & (__underlying_type(EnumClass))rhs); } \
                                                inline constexpr EnumClass  operator^ (EnumClass  lhs, EnumClass rhs)   { return (EnumClass)((__underlying_type(EnumClass))lhs ^ (__underlying_type(EnumClass))rhs); } \
                                                inline constexpr bool       operator! (EnumClass  e)                    { return !(__underlying_type(EnumClass))e; } \
                                                inline constexpr EnumClass  operator~ (EnumClass  e)                    { return (EnumClass)~(__underlying_type(EnumClass))e; }

namespace Horizon
{
    template<typename EnumClass>
    constexpr bool EnumClassHasFlags(EnumClass a, EnumClass b)
    {
        using UnderlyingType = __underlying_type(EnumClass);
        return ((UnderlyingType)a & (UnderlyingType)b) == (UnderlyingType)b;
    }
}