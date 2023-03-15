// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#ifndef EJSON_CONFIG_H_INCLUDED
#define EJSON_CONFIG_H_INCLUDED

/// If defined, indicates that json library is embedded in CppTL library.
//# define EJSON_IN_CPPTL 1

/// If defined, indicates that json may leverage CppTL library
//#  define EJSON_USE_CPPTL 1
/// If defined, indicates that cpptl vector based map should be used instead of
/// std::map
/// as Value container.
//#  define EJSON_USE_CPPTL_SMALLMAP 1

// If non-zero, the library uses exceptions to report bad input instead of C
// assertion macros. The default is to use exceptions.
#ifndef EJSON_USE_EXCEPTION
#define EJSON_USE_EXCEPTION 1
#endif

/// If defined, indicates that the source file is amalgated
/// to prevent private header inclusion.
/// Remarks: it is automatically defined in the generated amalgated header.
// #define EJSON_IS_AMALGAMATION

#ifdef EJSON_IN_CPPTL
#include <cpptl/config.h>
#ifndef EJSON_USE_CPPTL
#define EJSON_USE_CPPTL 1
#endif
#endif

#ifdef EJSON_IN_CPPTL
#define EJSON_API CPPTL_API
#elif defined(EJSON_DLL_BUILD)
#if defined(_MSC_VER)
#define EJSON_API __declspec(dllexport)
#endif // if defined(_MSC_VER)
#elif defined(EJSON_DLL)
#if defined(_MSC_VER)
#define EJSON_API __declspec(dllimport)
#endif // if defined(_MSC_VER)
#endif // ifdef EJSON_IN_CPPTL
#if !defined(EJSON_API)
#define EJSON_API
#endif

// If EJSON_NO_INT64 is defined, then Json only support C++ "int" type for
// integer
// Storages, and 64 bits integer support is disabled.
// #define EJSON_NO_INT64 1

#if defined(_MSC_VER) && _MSC_VER >= 1500 // MSVC 2008
/// Indicates that the following function is deprecated.
#define JSONCPP_DEPRECATED(message) __declspec(deprecated(message))
#elif defined(__clang__) && defined(__has_feature)
#if __has_feature(attribute_deprecated_with_message)
#define JSONCPP_DEPRECATED(message)  __attribute__ ((deprecated(message)))
#endif
#elif defined(__GNUC__) &&  (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#define JSONCPP_DEPRECATED(message)  __attribute__ ((deprecated(message)))
#elif defined(__GNUC__) &&  (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
#define JSONCPP_DEPRECATED(message)  __attribute__((__deprecated__))
#endif

#if !defined(JSONCPP_DEPRECATED)
#define JSONCPP_DEPRECATED(message)
#endif // if !defined(JSONCPP_DEPRECATED)

namespace EJson {
typedef int Int;
typedef unsigned int UInt;
#if defined(EJSON_NO_INT64)
typedef int LargestInt;
typedef unsigned int LargestUInt;
#undef EJSON_HAS_INT64
#else                 // if defined(EJSON_NO_INT64)
// For Microsoft Visual use specific types as long long is not supported
#if defined(_MSC_VER) // Microsoft Visual Studio
typedef __int64 Int64;
typedef unsigned __int64 UInt64;
#else                 // if defined(_MSC_VER) // Other platforms, use long long
typedef long long int Int64;
typedef unsigned long long int UInt64;
#endif // if defined(_MSC_VER)
typedef Int64 LargestInt;
typedef UInt64 LargestUInt;
#define EJSON_HAS_INT64
#endif // if defined(EJSON_NO_INT64)
} // end namespace EJson

#endif // EJSON_CONFIG_H_INCLUDED
