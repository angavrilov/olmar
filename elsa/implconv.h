// implconv.h                       see license.txt for copyright and terms of use
// implicit conversion sequences: cppstd 13.3.3.1, 13.3.3.2

// implicit conversions are those conversions possible in the
// absence of explicit conversion or cast syntax

#ifndef IMPLCONV_H
#define IMPLCONV_H

#include "stdconv.h"     // StandardConversion

class ImplicitConversion {
public:    // data
  enum Kind {
    IC_NONE,             // no conversion possible
    IC_STANDARD,         // 13.3.3.1.1: standard conversion sequence
    IC_USER_DEFINED,     // 13.3.3.1.2: user-defined conversion sequence
    IC_ELLIPSIS,         // 13.3.3.1.3: ellipsis conversion sequence
    IC_AMBIGUOUS,        // 13.3.3.1 para 10
    NUM_KINDS
  } kind;

  // for IC_STANDARD, this is the conversion sequence
  // for IC_USER_DEFINED, this is the *first* conversion sequence
  StandardConversion scs;       // "standard conversion sequence"

  // for IC_USER_DEFINED
  Variable const *user;         // the ctor or conversion operator function
  StandardConversion scs2;      // second conversion sequence (convert return value of 'user' to param type)

public:    // funcs
  ImplicitConversion()
    : kind(IC_NONE), scs(SC_IDENTITY), user(NULL), scs2(SC_IDENTITY) {}
  ImplicitConversion(ImplicitConversion const &obj)
    : DMEMB(kind), DMEMB(scs), DMEMB(user), DMEMB(scs2) {}

  // for determining whether the conversion attempt succeeded
  operator bool () const { return kind != IC_NONE; }

  // add specific conversion possibilities; automatically kicks
  // over to IC_AMBIGUOUS if there's already a conversion
  void addStdConv(StandardConversion scs);
  void addUserConv(StandardConversion first, Variable const *user, 
                   StandardConversion second);
  void addEllipsisConv();
};


ImplicitConversion getImplicitConversion(
  Env &env,            // type checking environment
  SpecialExpr special, // properties of the source expression
  Type const *src,     // source type
  Type const *dest     // destination type
);






#endif // IMPLCONV_H
