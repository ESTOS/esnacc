// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#ifndef CPPTL_EJSON_ASSERTIONS_H_INCLUDED
#define CPPTL_EJSON_ASSERTIONS_H_INCLUDED

#include <stdlib.h>
#include <sstream>

#if !defined(EJSON_IS_AMALGAMATION)
#include "config.h"
#endif // if !defined(EJSON_IS_AMALGAMATION)

/** It should not be possible for a maliciously designed file to
 *  cause an abort() or seg-fault, so these macros are used only
 *  for pre-condition violations and internal logic errors.
 */
#if EJSON_USE_EXCEPTION

// @todo <= add detail about condition in exception
# define EJSON_ASSERT(condition)                                                \
  {if (!(condition)) {EJson::throwLogicError( "assert json failed" );}}

# define EJSON_FAIL_MESSAGE(message)                                            \
  {                                                                            \
    std::ostringstream oss; oss << message;                                    \
    EJson::throwLogicError(oss.str());                                          \
  }

#else // EJSON_USE_EXCEPTION

# define EJSON_ASSERT(condition) assert(condition)

// The call to assert() will show the failure message in debug builds. In
// release builds we abort, for a core-dump or debugger.
# define EJSON_FAIL_MESSAGE(message)                                            \
  {                                                                            \
    std::ostringstream oss; oss << message;                                    \
    assert(false && oss.str().c_str());                                        \
    abort();                                                                   \
  }


#endif

#define EJSON_ASSERT_MESSAGE(condition, message)                                \
  if (!(condition)) {                                                          \
    EJSON_FAIL_MESSAGE(message);                                                \
  }

#endif // CPPTL_EJSON_ASSERTIONS_H_INCLUDED
