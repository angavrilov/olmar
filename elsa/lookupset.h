// lookupset.h
// LookupSet, a set of lookup candidates

#ifndef LOOKUPSET_H
#define LOOKUPSET_H

#include "sobjlist.h"        // SObjList

class Variable;              // variable.h


// variable lookup sometimes has complicated exceptions or
// special cases, so I'm folding lookup options into one value
enum LookupFlags {
  LF_NONE              = 0,
  LF_INNER_ONLY        = 0x00000001,   // only look in the innermost scope
  LF_ONLY_TYPES        = 0x00000002,   // ignore (skip over) non-type names
  LF_TYPENAME          = 0x00000004,   // user used 'typename' keyword
  LF_SKIP_CLASSES      = 0x00000008,   // skip class scopes
  LF_ONLY_NAMESPACES   = 0x00000010,   // ignore non-namespace names
  LF_TYPES_NAMESPACES  = 0x00000020,   // ignore non-type, non-namespace names
  LF_QUALIFIED         = 0x00000040,   // context is a qualified lookup
  LF_TEMPL_PRIMARY     = 0x00000080,   // return template primary rather than instantiating it
  LF_FUNCTION_NAME     = 0x00000100,   // looking up the name at a function call site
  LF_DECLARATOR        = 0x00000200,   // context is a declarator name lookup; for templates, this means to pick the primary or a specialization, but don't instantiate
  LF_SELFNAME          = 0x00000400,   // the DF_SELFNAME is visible
  LF_DEPENDENT         = 0x00000800,   // the lookup is in a dependent context
  LF_TEMPL_PARAM       = 0x00001000,   // return only template parameter/argument names
  LF_SUPPRESS_ERROR    = 0x00002000,   // during lookup, don't emit errors
  LF_SUPPRESS_NONEXIST = 0x00004000,   // suppress "does not exist" errors
  LF_IGNORE_USING      = 0x00008000,   // 3.4.2p3: ignore using-directives
  LF_NO_IMPL_THIS      = 0x00010000,   // do not insert implicit 'this->'
  LF_LOOKUP_SET        = 0x00020000,   // lookup can return a set of names

  LF_ALL_FLAGS         = 0x0003FFFF,   // bitwise OR of all flags
};

ENUM_BITWISE_OPS(LookupFlags, LF_ALL_FLAGS)     // smbase/macros.h


// filter a Variable w.r.t. a given set of flags, returning NULL
// if the Variable does not pass through the filter  
Variable *vfilter(Variable *v, LookupFlags flags);


// set of lookup candidates; equivalence classes are as identified
// by 'sameEntity' (declared in variable.h)
class LookupSet : public SObjList<Variable> {
private:     // disallowed
  LookupSet(LookupSet&);
  void operator= (LookupSet&);
  void operator== (LookupSet&);

public:
  LookupSet();
  ~LookupSet();

  // like vfilter, but also accumulate the Variable in the set
  // if LF_LOOKUP_SET is in 'flags'
  Variable *filter(Variable *v, LookupFlags flags);

  // add 'v' to a candidate set, such that the set has exactly one
  // entry for each unique entity; this breaks apart 'v' if it is
  // an overload set and enters each overloaded entity separately
  void adds(Variable *v);

  // same as above except add 'v' itself, ignoring whether it
  // is an overload set
  void add(Variable *v);
  
  // like 'adds' but only do it if LF_LOOKUP_SET is in 'flags'
  void addsIf(Variable *v, LookupFlags flags);
};


#endif // LOOKUPSET_H
