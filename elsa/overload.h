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


// debugging output support
extern int overloadNesting;      // overload resolutions ongoing

// ostream with line prefix already printed
ostream &overloadTrace();

#ifndef NDEBUG
  class OverloadIndTrace {
  public:
    OverloadIndTrace(char const *msg) {
      overloadTrace() << msg << endl;
      overloadNesting++;
    }
    ~OverloadIndTrace() {
      overloadNesting--;
    }
  };

  // print a message, indent, and at the end of this function,
  // outdent automatically
  #define OVERLOADINDTRACE(msg) \
    OverloadIndTrace otrace(stringc << msg);

  // just print a message at the current indentation
  #define OVERLOADTRACE(msg) \
    overloadTrace() << msg << endl

#else
  #define OVERLOADINDTRACE(msg) ((void)0)
  #define OVERLOADTRACE(msg) ((void)0)
#endif


// information about an argument expression, for use with
// overload resolution
class ArgumentInfo {
public:
  SpecialExpr special;          // whether it's a special expression
  Type *type;                   // type of argument

public:
  ArgumentInfo()
    : special(SE_NONE), type(NULL) {}
  ArgumentInfo(SpecialExpr s, Type *t)
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
  void conversionDescriptions() const;
};


// flags to control overload resolution
enum OverloadFlags {
  OF_NONE        = 0x00,           // nothing special
  OF_NO_USER     = 0x01,           // don't consider user-defined conversions
  OF_NO_EXPLICIT = 0x02,           // disregard DF_EXPLICIT Variables
  OF_ALL         = 0x03,           // all flags
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
  GrowArray<ArgumentInfo> &args;
  
  // when non-NULL, this indicates the type of the expression
  // that is being copy-initialized, and plays a role in selecting
  // the best function (13.3.3, final bullet)
  Type *finalDestType;

  // these are the "viable candidate functions" of the standard
  ObjArrayStack<Candidate> candidates;

private:     // funcs
  Candidate * /*owner*/ makeCandidate(Variable *var);
  void printArgInfo();

public:      // funcs
  OverloadResolver(Env &en, SourceLoc L, ErrorList *er,
                   OverloadFlags f, GrowArray<ArgumentInfo> &a,
                   int numCand = 10 /*estimate of # of candidates*/)
    : env(en),
      loc(L),
      errors(er),
      flags(f),
      args(a),
      finalDestType(NULL),

      // this estimate does not have to be perfect; if it's high,
      // then more space will be allocated than necessary; if it's
      // low, then the 'candidates' array will have to be resized
      // at some point; it's entirely a performance issue
      candidates(numCand)
  {
    //overloadNesting++;
    printArgInfo();
  }
  ~OverloadResolver();

  // public for 'tournament'
  int compareCandidates(Candidate const *left, Candidate const *right);

  // process a batch of candidate functions, adding the viable
  // ones to the 'candidates' list
  void processCandidates(SObjList<Variable> &varList);
  void processCandidate(Variable *v);

  // if 'v' has an overload set, then process that; otherwise, just
  // process 'v' alone
  void processPossiblyOverloadedVar(Variable *v);

  // run the tournament to decide among the candidates; returns
  // NULL if there is no clear winner
  Variable *resolve(bool &wasAmbig);
  Variable *resolve();     // ignore ambiguity info
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
  GrowArray<ArgumentInfo> &args,   // list of argument types at the call site
  bool &wasAmbig                   // returns as true if error due to ambiguity
);


// collect the set of conversion operators that 'ct' has; this
// interface will change once I get a proper implementation of
// conversion operator inheritance
void getConversionOperators(SObjList<Variable> &dest, Env &env,
                            CompoundType *ct);


// given an object of type 'srcClass', find a conversion operator
// that will yield 'destType' (perhaps with an additional standard
// conversion); for now, this function assumes the conversion
// context is as in 13.3.1.{4,5,6}: copy-initialization by conversion
// (NOTE: this does *not* try "converting constructors" of 'destType')
ImplicitConversion getConversionOperator(
  Env &env,
  SourceLoc loc,
  ErrorList * /*nullable*/ errors,
  Type *srcClassType,      // must be a compound (or reference to one)
  Type *destType
);


// least upper bound: given two pointer types T1 and T2, compute the
// unique type S such that:
//   (a) T1 and T2 can be standard-converted to S
//   (b) for any other type S' != S that T1 and T2 can be
//       standard-converted to, the conversion T1->S is better than
//       T1->S' or T2->S is better than T2->S', and neither ->S
//       conversion is worse than a ->S' conversion
// if no type satisfies (a) and (b), return NULL; furthermore, if
// a type satisfies (a) but not (b), then yield 'wasAmbig'
Type *computeLUB(Env &env, Type *t1, Type *t2, bool &wasAmbig);

// test vector for 'computeLUB'; code:
//   0=fail
//   1=success, should match 'answer'
//   2=ambiguous
void test_computeLUB(Env &env, Type *t1, Type *t2, Type *answer, int code);


#endif // OVERLOAD_H
