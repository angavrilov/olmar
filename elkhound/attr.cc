// attr.cc
// code for attr.h

#include "attr.h"     // this module


// --------------------- Attributes::Entry ------------------------
Attributes::Entry::~Entry()
{}

void Attributes::Entry::print(ostream &os) const
{
  os << name << "=" << value;
}

STATICDEF int Attributes::Entry::compare(
  Entry const *left, Entry const *right, void*)
{
  int rel = left->name.compareTo(right->name);
  if (rel == 0) {			   
    // checking values means when we compare two attribute sets
    // we can use this 'compare' fn there as well
    rel = left->value - right->value;
  }
  return rel;
}


// ----------------------- Attributes -----------------------------
Attributes::Attributes()
{}

Attributes::~Attributes()
{}


// locate an entry by name, return NULL if isn't there
Attributes::Entry const *Attributes::findEntryC(char const *name) const
{
  FOREACH_OBJLIST(Entry, dict, entry) {
    if (entry.data()->name.equals(name)) {
      return entry.data();
    }
  }
  return NULL;
}


bool Attributes::hasEntry(char const *name) const
{
  return findEntryC(name) != NULL;
}


int Attributes::get(char const *name) const
{
  Entry const *e = findEntryC(name);
  if (!e) {
    xfailure(stringc << "attribute `" << name << "' doesn't have a value");
  }
  return e->value;
}


void Attributes::set(char const *name, int value)
{
  if (!hasEntry(name)) {
    addEntry(name, value);
  }
  else {
    changeEntry(name, value);
  }
}


void Attributes::addEntry(char const *name, int value)
{
  xassert(!hasEntry(name));

  // we keep them in sorted order to faciliate comparisons
  dict.insertSorted(new Entry(name, value), Entry::compare);
}


void Attributes::changeEntry(char const *name, int value)
{
  Entry *e = findEntry(name);
  xassert(e);
  e->value = value;

  // since the sort criteria depends on value, this might seem to
  // change the sorted order; but it does not, since we never allow
  // the list to contain more than one entry with a given name
  // (so the value check is never invoked during sorting)
}


void Attributes::removeEntry(char const *name)
{
  Entry *e = findEntry(name);
  xassert(e);
  dict.removeItem(e);
  delete e;
}


bool Attributes::operator==(Attributes const &obj) const
{
  return dict.equalAsLists(obj.dict, Entry::compare);
}


void Attributes::destructiveEquals(Attributes &obj)
{
  // we're not checking for collisions, so this is necessary
  xassert(dict.isEmpty());

  dict.concat(obj.dict);
}


void Attributes::print(ostream &os) const
{
  os << "{";
  int ct=0;
  FOREACH_OBJLIST(Entry, dict, iter) {
    if (ct++ > 0) {
      os << ",";
    }
    os << " " << *(iter.data());
  }
  os << " }";
}
