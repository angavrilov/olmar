// objlist.h
// owner list of arbitrary dynamically-allocated objects

#ifndef __OBJLIST_H
#define __OBJLIST_H

#include "voidlist.h"    // VoidList


// forward declarations of template classes, so we can befriend them in ObjList
// (not required by Borland C++ 4.5, but GNU wants it...)
template <class T> class ObjListIter;
template <class T> class ObjListMutator;


// the list is considered to own all of the items; it is an error to insert
// an item into more than one such list, or to insert an item more than once
// into any such list
template <class T>
class ObjList {
private:
  friend class ObjListIter<T>;
  friend class ObjListMutator<T>;

protected:
  VoidList list;                        // list itself

private:
  ObjList(ObjList const &obj) {}        // not allowed

public:
  ObjList()                             :list() {}
  ~ObjList()                            { deleteAll(); }

  // The difference function should return <0 if left should come before
  // right, 0 if they are equivalent, and >0 if right should come before
  // left.  For example, if we are sorting numbers into ascending order,
  // then 'diff' would simply be subtraction.
  typedef int (*Diff)(T *left, T *right, void *extra);

  // selectors
  int count() const                     { return list.count(); }
  bool isEmpty() const                  { return list.isEmpty(); }

  T *nth(int which)                     { return (T*)list.nth(which); }
  T const *nthc(int which) const        { return (T const*)list.nth(which); }

  bool contains(T const *item) const    { return list.contains(item); }
  int indexOf(T const *item) const      { return list.indexOf(item); }

  // insertion
  void prepend(T *newitem)              { list.prepend(newitem); }
  void append(T *newitem)               { list.append(newitem); }
  void insertAt(T *newitem, int index)	{ list.insertAt(newitem, index); }

  // removal
  T *removeAt(int index)                { return (T*)list.removeAt(index); }
  void deleteAt(int index)              { delete (T*)list.removeAt(index); }
  void deleteAll();

  // complex modifiers
  void insertionSort(Diff diff, void *extra=NULL)   { list.insertionSort((VoidDiff)diff, extra); }
  void mergeSort(Diff diff, void *extra=NULL)       { list.mergeSort((VoidDiff)diff, extra); }

  // multiple lists
  void concat(ObjList &tail)            { list.concat((VoidList&)tail); }

  // debugging
  bool invariant() const                { return list.invariant(); }
};


template <class T>
void ObjList<T>::deleteAll()
{
  while (!list.isEmpty()) {
    deleteAt(0);
  }
}


// for traversing the list without modifying it (neither nodes nor structure)
// NOTE: no list-modification fns should be called on 'list' while this
//       iterator exists
template <class T>
class ObjListIter {
protected:
  VoidListIter iter;	  // underlying iterator

public:
  ObjListIter(ObjList<T> const &list) : iter(list.list) {}
  ~ObjListIter()                      {}

  void reset(ObjList<T> const &list)  { iter.reset(list.list); }

  // iterator actions
  bool isDone() const                 { return iter.isDone(); }
  void adv()                          { iter.adv(); }
  T const *data() const	       	      { return (T const*)iter.data(); }
};


// for traversing the list and modifying it (nodes and/or structure)
// NOTE: no list-modification fns should be called on 'list' while this
//       iterator exists, and only one such iterator should exist for
//       any given list
template <class T>
class ObjListMutator {
protected:
  VoidListMutator mut;       // underlying mutator

public:
  ObjListMutator(ObjList<T> &lst)       : mut(lst.list) { reset(); }
  ~ObjListMutator()                     {}

  void reset()                          { mut.reset(); }

  // iterator actions
  bool isDone() const                   { return mut.isDone(); }
  void adv()                            { mut.adv(); }
  T *data()                             { return (T*)mut.data(); }

  // insertion
  void insertBefore(T *item)            { mut.insertBefore(item); }
    // 'item' becomes the new 'current', and the current 'current' is
    // pushed forward (so the next adv() will make it current again)

  void insertAfter(T *item)             { mut.insertAfter(item); }
    // 'item' becomes what we reach with the next adv();
    // isDone() must be false

  void append(T *item)                  { mut.append(item); }
    // only valid while isDone() is true, it inserts 'item' at the end of
    // the list, and advances such that isDone() remains true; equivalent
    // to { xassert(isDone()); insertBefore(item); adv(); }

  // removal
  T *remove()                           { return (T*)mut.remove(); }
    // 'current' is removed from the list and returned, and whatever was
    // next becomes the new 'current'

  void deleteIt()                       { delete (T*)mut.remove(); }
    // same as remove(), except item is deleted also

  // debugging
  bool invariant() const                { return mut.invariant(); }
};

#endif // __OBJLIST_H
