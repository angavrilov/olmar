// vdtllist.h
// list of void*, with a pointer maintained to the last (tail)
// element, for constant-time append

#ifndef VDTLLIST_H
#define VDTLLIST_H

#include "voidlist.h"      // VoidList

// inherit privately so I can choose what to expose
class VoidTailList : private VoidList {
private:
  // by making this a friend, it should see VoidList as a
  // base class, and thus simply work
  friend VoidListIter;
  
  // no mutator for now

protected:
  VoidNode *tail;       // (serf) last element of list, or NULL if list is empty

private:
  VoidTailList(VoidTailList const &obj);    // not allowed

public:
  VoidTailList()                     { tail = NULL; }
  ~VoidTailList()                    {}

  // this syntax just makes the implementation inherited from
  // 'VoidList' public, whereas it would default to private,
  // since it was inherited privately
  VoidList::count;

  // see voidlist.h for documentation of each of these functions
  VoidList::isEmpty;
  VoidList::isNotEmpty;
  VoidList::nth;
  VoidList::first;
  void *last() const                 { xassert(tail); return tail; }

  // insertion
  void prepend(void *newitem);
  void append(void *newitem);
  void insertAt(void *newitem, int index);

  // removal
  //void *removeAt(int index);
  void *removeFirst();               // remove first, return data; must exist
  void removeAll();

  // list-as-set: selectors
  VoidList::indexOf;
  VoidList::indexOfF;
  VoidList::contains;

  // list-as-set: mutators
  bool prependUnique(void *newitem);
  bool appendUnique(void *newitem);
  //void removeItem(void *item);
  //bool removeIfPresent(void *item);

  // debugging
  void selfCheck() const;
  VoidList::debugPrint;
};


#endif // VDTLLIST_H
