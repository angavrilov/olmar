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
class ErrorList;


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
  ~Candidate();
                                        
  // debugging
  string conversionDescriptions(char const *indent) const;
};


// flags to control overload resolution (there used to be more than one...)
enum OverloadFlags {
  OF_NONE        = 0x00,           // nothing special
  OF_NO_USER     = 0x01,           // don't consider user-defined conversions
  OF_ALL         = 0x01,           // all flags
};

ENUM_BITWISE_OPS(OverloadFlags, OF_ALL);


// this class implements a single overload resolution, exposing
// a richer interface than the simple 'resolveOverload' call below
class OverloadResolver {
public:      // data
  // same meaning as corresponding arguments to 'resolveOverload'
  Env &env;
  SourceLoc loc;
  ErrorList * /*nullable*/ errors;
  OverloadFlags flags;          
  // no list of candidates here
  GrowArray<ArgumentInfo> &args;

  // these are the "viable functions" of the standard
  ObjArrayStack<Candidate> candidates;

private:     // funcs
  Candidate * /*owner*/ makeCandidate(Variable *var);
  Candidate *pickWinner(int low, int high);
  int compareCandidates(Candidate const *left, Candidate const *right);

public:      // funcs
  OverloadResolver(Env &en, SourceLoc L, ErrorList *er,
                   OverloadFlags f, GrowArray<ArgumentInfo> &a,
                   int numCand /*estimate of # of candidates*/)
    : env(en),
      loc(L),
      errors(er),
      flags(f),
      args(a),
      candidates(numCand)
  {}
  ~OverloadResolver();

  // process a batch of candidate functions, adding the viable
  // ones to the 'candidates' list
  void processCandidates(SObjList<Variable> &varList);
  void processCandidate(Variable *v);

  // run the tournament to decide among the candidates; returns
  // NULL if there is no clear winner
  Variable *resolve();
};


// resolve the overloading, return the selected candidate; if nothing
// matches or there's an ambiguity, adds an error to 'env' and returns
// NULL
Variable *resolveOverload(
  Env &env,                        // environment in which to perform lookups
  SourceLoc loc,                   // location for error reports
  ErrorList * /*nullable*/ errors, // where to insert errors; if NULL, don't
  OverloadFlags flags,             // various options
  SObjList<Variable> &list,        // list of overloaded possibilities
  GrowArray<ArgumentInfo> &args);  // list of argument types at the call site



#endif // OVERLOAD_H
