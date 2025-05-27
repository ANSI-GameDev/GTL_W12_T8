#pragma once
#include <type_traits>

template<typename Enum>
constexpr bool EnumHasAnyFlags(Enum Flags, Enum Contains)
{
    using UnderlyingType = __underlying_type(Enum);
    return ((UnderlyingType)Flags & (UnderlyingType)Contains) != 0;
}

#define ENUM_CLASS_FLAGS(Enum) \
inline Enum operator|(Enum A, Enum B) { return static_cast<Enum>(static_cast<std::underlying_type<Enum>::type>(A) | static_cast<std::underlying_type<Enum>::type>(B)); } \
inline Enum operator&(Enum A, Enum B) { return static_cast<Enum>(static_cast<std::underlying_type<Enum>::type>(A) & static_cast<std::underlying_type<Enum>::type>(B)); } \
inline Enum operator~(Enum A) { return static_cast<Enum>(~static_cast<std::underlying_type<Enum>::type>(A)); } \
inline Enum& operator|=(Enum& A, Enum B) { A = A | B; return A; } \
inline Enum& operator&=(Enum& A, Enum B) { A = A & B; return A; } \
inline bool EnumHasAnyFlags(Enum Flags, Enum Test) { return static_cast<std::underlying_type<Enum>::type>(Flags & Test) != 0; }
