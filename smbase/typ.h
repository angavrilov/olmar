// typ.h
// various types and definitions, some for portability, others for convenience
// Scott McPeak, 1996-2000  This file is public domain.

#ifndef __TYP_H
#define __TYP_H

// byte
typedef unsigned char byte;
typedef signed char signed_byte;


// int32 used to be here, but defined nonportably, and I don't use
// it anyway, so I ripped it out


// NULL
#ifndef NULL
#  define NULL 0
#endif // NULL


// bool
#ifdef LACKS_BOOL
  typedef int bool;
  bool const false=0;
  bool const true=1;
#endif // LACKS_BOOL


// min, max
#ifndef __MINMAX_DEFINED
# ifndef min
#  define min(a,b) ((a)<(b)?(a):(b))
# endif
# ifndef max
#  define max(a,b) ((a)>(b)?(a):(b))
# endif
# define __MINMAX_DEFINED
#endif // __MINMAX_DEFINED


// tag for definitions of static member functions; there is no
// compiler in existence for which this is useful, but I like
// to see *something* next to implementations of static members
// saying that they are static, and this seems slightly more
// formal than just a comment
#define STATICDEF /*static*/


// often-useful number-of-entries function
#define TABLESIZE(tbl) ((int)(sizeof(tbl)/sizeof((tbl)[0])))


// concise way to loop on an integer range
#define loopi(end) for(int i=0; i<(int)(end); i++)
#define loopj(end) for(int j=0; j<(int)(end); j++)
#define loopk(end) for(int k=0; k<(int)(end); k++)


// for using selfCheck methods
// to explicitly check invariants in debug mode
#ifndef NDEBUG
#  define SELFCHECK() selfCheck()
#else
#  define SELFCHECK() ((void)0)
#endif


// division with rounding towards +inf
// (when operands are positive)
template <class T>
inline T div_up(T const &x, T const &y)
{ return (x + y - 1) / y; }


// mutable
#ifdef __BORLANDC__
#  define MUTABLE
#  define EXPLICIT
#else
#  define MUTABLE mutable
#  define EXPLICIT explicit
#endif


#define SWAP(a,b) \
  temp = a;       \
  a = b;          \
  b = temp /*user supplies semicolon*/

  
// verify something is true at compile time (will produce
// a compile error if it isn't)
#define staticAssert(cond) extern int dummyArray[cond? 1 : 0]


#endif // __TYP_H

