// xassert.h
// replacement for assert that throws an exception on failure
// (x_assert_fail and xfailure_func are in exc.cc)
// Scott McPeak, 1997-1998  This file is public domain.

#ifndef __XASSERT_H
#define __XASSERT_H

#include "macros.h"     // NORETURN

void x_assert_fail(char const *cond, char const *file, int line) NORETURN;

#if !defined(NDEBUG_NO_ASSERTIONS)
  #define xassert(cond) \
    ((cond)? (void)0 : x_assert_fail(#cond, __FILE__, __LINE__))
#else
  #define xassert(cond) ((void)0)
#endif

// here's a version which will turn off with ordinary NDEBUG
#if !defined(NDEBUG)
  #define xassertdb(cond) xassert(cond)
#else
  #define xassertdb(cond) ((void)0)
#endif

// call when state is known to be bad; will *not* return (ideal
// behavior is to throw an exception, systems lacking this can
// call abort())
#define xfailure(why) x_assert_fail(why, __FILE__, __LINE__)


// quick note: one prominent book on writing code recommends that
// assertions *not* include the failure condition, since the file
// and line number are sufficient, and the condition string uses
// memory.  the problem is that sometimes a compiled binary is
// out of date w/respect to the code, and line numbers move, so
// the condition string provides a good way to find the right
// assertion


#endif // __XASSERT_H

