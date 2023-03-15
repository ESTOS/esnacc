// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#ifndef EJSON_FORWARDS_H_INCLUDED
#define EJSON_FORWARDS_H_INCLUDED

#if !defined(EJSON_IS_AMALGAMATION)
#include "config.h"
#endif // if !defined(EJSON_IS_AMALGAMATION)

namespace EJson {

// writer.h
class FastWriter;
class StyledWriter;

// reader.h
class Reader;

// features.h
class Features;

// value.h
typedef unsigned int ArrayIndex;
class StaticString;
class Path;
class PathArgument;
class Value;
class ValueIteratorBase;
class ValueIterator;
class ValueConstIterator;

} // namespace EJson

#endif // EJSON_FORWARDS_H_INCLUDED
