// lookupset.cc
// code for lookupset.h

#include "lookupset.h"        // this module
#include "variable.h"         // Variable, sameEntity


// vfilter: variable filter
// implements variable-filtering aspect of the flags; the idea
// is you never query 'variables' without wrapping the call
// in a filter
Variable *vfilter(Variable *v, LookupFlags flags)
{
  if (!v) return v;

  if ((flags & LF_ONLY_TYPES) &&
      !v->hasFlag(DF_TYPEDEF)) {
    return NULL;
  }

  if ((flags & LF_ONLY_NAMESPACES) &&
      !v->hasFlag(DF_NAMESPACE)) {
    return NULL;
  }

  if ((flags & LF_TYPES_NAMESPACES) &&
      !v->hasFlag(DF_TYPEDEF) &&
      !v->hasFlag(DF_NAMESPACE)) {
    return NULL;
  }

  if (!(flags & LF_SELFNAME) &&
      v->hasFlag(DF_SELFNAME)) {
    // the selfname is not visible b/c LF_SELFNAME not specified
    return NULL;
  }
                                                        
  // I actually think it would be adequate to just check for
  // DF_BOUND_TARG...
  if ((flags & LF_TEMPL_PARAM) &&
      !v->hasAnyFlags(DF_BOUND_TARG | DF_TEMPL_PARAM)) {
    return NULL;
  }

  return v;
}


// --------------------- LookupSet ----------------------
LookupSet::LookupSet()
{}

LookupSet::~LookupSet()
{}


Variable *LookupSet::filter(Variable *v, LookupFlags flags)
{
  v = vfilter(v, flags);
  if (v) {
    addsIf(v, flags);
  }
  return v;
}


void LookupSet::adds(Variable *v)
{
  if (v->isOverloaded()) {
    SFOREACH_OBJLIST_NC(Variable, v->overload->set, iter) {
      add(iter.data());
    }
  }
  else {
    add(v);
  }
}

void LookupSet::add(Variable *v)
{
  // is 'v' already present?
  SFOREACH_OBJLIST(Variable, *this, iter) {
    if (sameEntity(v, iter.data())) {
      return;      // already present
    }
  }

  // not already present, add it
  prepend(v);
}


void LookupSet::addsIf(Variable *v, LookupFlags flags)
{
  if (flags & LF_LOOKUP_SET) {
    adds(v);
  }
}


// EOF
