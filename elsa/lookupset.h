// lookupset.h
// LookupSet, a set of lookup candidates

// I pulled this out into its own file so I could #include it
// in some places without running into circularity issues.

#ifndef LOOKUPSET_H
#define LOOKUPSET_H

#include "sobjlist.h"        // SObjList

class Variable;              // variable.h

// set of lookup candidates
typedef SObjList<Variable> LookupSet;

// add 'v' to a candidate set, such that the set has exactly one
// entry for each unique entity; this breaks apart 'v' if it is
// an overload set and enters each overloaded entity separately
void prependUniqueEntities(LookupSet &candidates, Variable *v);

// same as above except add 'v' itself, ignoring whether it
// is an overload set
void prependUniqueEntity(LookupSet &candidates, Variable *v);


#endif // LOOKUPSET_H
