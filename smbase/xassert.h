// xassert.h
// replacement for assert that throws an exception on failure
// (x_assert_fail and xfailure_func are in exc.cc)
// Scott McPeak, 1997-1998  This file is public domain.

#ifndef __XASSERT_H
#define __XASSERT_H


void x_assert_fail(char const *cond, char const *file, int line);

#if !defined(NDEBUG) || defined(NDEBUG_ASSERTIONS)
  #define xassert(cond) \
    ((cond)? (void)0 : x_assert_fail(#cond, __FILE__, __LINE__))
#else
  #define xassert(cond) ((void)0)
#endif


// call when state is known to be bad; will *not* return (ideal
// behavior is to throw an exception, systems lacking this can
// call abort())
#define xfailure(why) x_assert_fail(why, __FILE__, __LINE__)


#endif // __XASSERT_H

