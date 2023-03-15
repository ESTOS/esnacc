// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#ifndef EJSON_AUTOLINK_H_INCLUDED
#define EJSON_AUTOLINK_H_INCLUDED

#include "config.h"

#ifdef EJSON_IN_CPPTL
#include <cpptl/cpptl_autolink.h>
#endif

#if !defined(EJSON_NO_AUTOLINK) && !defined(EJSON_DLL_BUILD) &&                  \
    !defined(EJSON_IN_CPPTL)
#define CPPTL_AUTOLINK_NAME "json"
#undef CPPTL_AUTOLINK_DLL
#ifdef EJSON_DLL
#define CPPTL_AUTOLINK_DLL
#endif
#include "autolink.h"
#endif

#endif // EJSON_AUTOLINK_H_INCLUDED
