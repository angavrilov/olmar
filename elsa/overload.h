// overload.h                       see license.txt for copyright and terms of use
// attempt at implementing C++ overload resolution;
// see cppstd section ("clause") 13

#ifndef OVERLOAD_H
#define OVERLOAD_H

#include "sobjlist.h"      // SObjList
#include "array.h"         // ArrayStack
#include "implconv.h"      // ImplicitConversion, StandardConversion

// fwds
class Env;
class Variable;
class Type;


// information about an argument expression, for use with
// overload resolution
class ArgumentInfo {  
public:
  SpecialExpr special;          // whether it's a special expression
  Type const *type;             // type of argument

public:
  ArgumentInfo()
    : special(SE_NONE), type(NULL) {}
  ArgumentInfo(SpecialExpr s, Type const *t)
    : special(s), type(t) {}
  ArgumentInfo(ArgumentInfo const &obj)
    : DMEMB(special), DMEMB(type) {}
  ArgumentInfo& operator= (ArgumentInfo const &obj)
    { CMEMB(special); CMEMB(type); return *this; }
};


// resolve the overloading, return the selected candidate; if nothing
// matches or there's an ambiguity, adds an error to 'env' and returns
// one of the candidates arbitrarily (for error recovery)
Variable *resolveOverload(
  Env &env,                        // for emitting error messages, etc.
  SObjList<Variable> &list,        // list of overloaded possibilities
  GrowArray<ArgumentInfo> &args);  // list of argument types at the call site


// information about a single overload possibility
class Candidate {
public:
  // the candidate itself, with its type
  Variable *var;

  // list of conversions, one for each argument
  GrowArray<ImplicitConversion> conversions;

public:
  // here, 'numArgs' is the number of actual arguments, *not* the
  // number of parameters in var's function; it's passed so I know
  // how big to make 'conversions'
  Candidate(Variable *v, int numArgs);
};



#endif // OVERLOAD_H
