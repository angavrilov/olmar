// stdconv.h                       see license.txt for copyright and terms of use
// Standard Conversions, i.e. Section ("clause") 4 of cppstd

// A Standard Conversion is one of a dozen or so type coercions
// described in section 4 of cppstd.  A Standard Conversion Sequence
// is up to three Standard Conversions, drawn from particular
// subsets of such conversions, to be applied in sequence.

// Standard Conversions are the kinds of conversions that can be
// done implicitly (no explicit conversion or cast syntax), short
// of user-defined constructors and conversions.

// Note that in my system, there is nothing directly called an
// "lvalue", rather there are simply references and non-references.

#ifndef STDCONV_H
#define STDCONV_H

#include "macros.h"    // ENUM_BITWISE_AND,OR
#include "str.h"       // string
#include "cc_type.h"   // SpecialExpr

// fwd
class Type;            // cc_type.h
class Env;             // cc_env.h

// the kinds of Standard Conversions; any given pair of convertible
// types will be related by the conversions permitted as one or more
// of the following kinds; ORing together zero or one conversion from
// each group yields a Standard Conversion Sequence
enum StandardConversion {
  SC_IDENTITY        = 0x000,  // types are identical

  // conversion group 1 (comes first)
  SC_LVAL_TO_RVAL    = 0x001,  // 4.1: int& -> int
  SC_ARRAY_TO_PTR    = 0x002,  // 4.2: char[] -> char*
  SC_FUNC_TO_PTR     = 0x003,  // 4.3: int ()(int) -> int (*)(int)
  SC_GROUP_1_MASK    = 0x00F,

  // conversion group 3 (comes last conceptually)
  SC_QUAL_CONV       = 0x010,  // 4.4: int* -> int const*
  SC_GROUP_3_MASK    = 0x0F0,

  // conversion group 2 (goes in the middle)
  SC_INT_PROM        = 0x100,  // 4.5: int... -> int..., no info loss possible
  SC_FLOAT_PROM      = 0x200,  // 4.6: float -> double, no info loss possible
  SC_INT_CONV        = 0x300,  // 4.7: int... -> int..., info loss possible
  SC_FLOAT_CONV      = 0x400,  // 4.8: float... -> float..., info loss possible
  SC_FLOAT_INT_CONV  = 0x500,  // 4.9: int... <-> float..., info loss possible
  SC_PTR_CONV        = 0x600,  // 4.10: 0 -> Foo*, Child* -> Parent*
  SC_PTR_MEMB_CONV   = 0x700,  // 4.11: int Child::* -> int Parent::*
  SC_BOOL_CONV       = 0x800,  // 4.12: various types <-> bool
  SC_GROUP_2_MASK    = 0xF00,

  SC_ERROR           = 0xFFFF, // cannot convert
};

// for '&', one of the arguments should always be a mask
ENUM_BITWISE_AND(StandardConversion)                    

// for '|', some care should be taken to ensure you're not
// ORing together nonzero elements from the same group
ENUM_BITWISE_OR(StandardConversion)
                                               
// render in C++ syntax as bitwise OR of the constants above
string toString(StandardConversion c);


// remove SC_LVAL_TO_RVAL from a conversion sequence
StandardConversion removeLval(StandardConversion scs);

// true if 'left' is a subsequence of 'right'
bool isSubsequenceOf(StandardConversion left, StandardConversion right);


// standard conversion rank, as classified by cppstd Table 9
// (section 13.3.3.1.1 para 3)
enum SCRank {
  SCR_EXACT,
  SCR_PROMOTION,
  SCR_CONVERSION,
  NUM_SCRANKS
};

SCRank getRank(StandardConversion scs);


// given two types, determine the Standard Conversion Sequence,
// if any, that will convert 'src' into 'dest'
StandardConversion getStandardConversion(
  Env *env,            // if non-null, failed conversion inserts error message
  SpecialExpr special, // properties of the source expression
  Type const *src,     // source type
  Type const *dest     // destination type
);


// testing interface, for use by the type checker
void test_getStandardConversion(
  Env &env, SpecialExpr special, Type const *src, Type const *dest,
  int expected     // expected return value
);


#endif // STDCONV_H
