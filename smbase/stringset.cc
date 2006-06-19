// stringset.cc            see license.txt for copyright and terms of use
// code for stringset.h

#include "stringset.h"        // this module

StringSet::~StringSet()
{}

void StringSet::add(char const *elt)
{
  if (!contains(elt)) {
    elts.add(elt, NULL);
  }
}

void StringSet::remove(char const *elt)
{
  if (contains(elt)) {
    elts.remove(elt);
  }
}

