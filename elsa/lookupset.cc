// lookupset.cc
// code for lookupset.h

#include "lookupset.h"        // this module
#include "variable.h"         // Variable, sameEntity


void prependUniqueEntities(LookupSet &candidates, Variable *v)
{
  if (v->isOverloaded()) {
    SFOREACH_OBJLIST_NC(Variable, v->overload->set, iter) {
      prependUniqueEntity(candidates, iter.data());
    }
  }
  else {
    prependUniqueEntity(candidates, v);
  }
}

void prependUniqueEntity(LookupSet &candidates, Variable *v)
{
  // is 'v' already present?
  SFOREACH_OBJLIST(Variable, candidates, iter) {
    if (sameEntity(v, iter.data())) {
      return;      // already present
    }
  }

  // not already present, add it
  candidates.prepend(v);
}


// EOF
