m4_dnl // xobjlist.h
m4_dnl // template file to be processed with m4 to generate one
m4_dnl // of wrappers around VoidList
m4_dnl
m4_changequote([, ])m4_dnl      // for this section
m4_changecom[]m4_dnl            // no m4 "comments"
m4_ifelse(m4_output, sobjlist.h, [m4_dnl
// sobjlist.h
// serf list of arbitrary objects
m4_define(makeName, S[$1])m4_dnl
m4_define(outputCond, [$1])m4_dnl       // select 1st arg
], [m4_dnl
// objlist.h
// owner list of arbitrary dynamically-allocated objects
m4_define(makeName, [$1])m4_dnl
m4_define(outputCond, [$2])m4_dnl       // select 2nd arg
])m4_dnl
m4_define(includeLatch, makeName(OBJLIST_H))m4_dnl
m4_define(className, makeName(ObjList))m4_dnl
m4_define(iterName, makeName(ObjListIter))m4_dnl
m4_define(mutatorName, makeName(ObjListMutator))m4_dnl
m4_changequote(, )m4_dnl              // so quotes are not quoted..
m4_changequote([[[, ]]])m4_dnl        // reduce likelihood of confusion
// NOTE: automatically generated from xobjlist.h -- do not edit directly

// Author: Scott McPeak, 2000

#ifndef includeLatch
#define includeLatch

#include "voidlist.h"    // VoidList


// forward declarations of template classes, so we can befriend them in className
// (not required by Borland C++ 4.5, but GNU wants it...)
template <class T> class iterName;
template <class T> class mutatorName;


outputCond([[[m4_dnl      // sobjlist
// the list is considered to not own any of the items; it's ok to
// insert items multiple times or into multiple lists
]]], [[[m4_dnl            // objlist
// the list is considered to own all of the items; it is an error to insert
// an item into more than one such list, or to insert an item more than once
// into any such list
]]])m4_dnl
template <class T>
class className {
private:
  friend class iterName<T>;
  friend class mutatorName<T>;

protected:
  VoidList list;                        // list itself

private:
  className[[[]]](className const &obj) {}      // not allowed

public:
  className[[[]]]()                            :list() {}
  ~className[[[]]]()                        m4_dnl
     outputCond({}    /* all items removed */, { deleteAll(); })

  // The difference function should return <0 if left should come before
  // right, 0 if they are equivalent, and >0 if right should come before
  // left.  For example, if we are sorting numbers into ascending order,
  // then 'diff' would simply be subtraction.
  typedef int (*Diff)(T const *left, T const *right, void *extra);

  // selectors
  int count() const                     { return list.count(); }
  bool isEmpty() const                  { return list.isEmpty(); }
  bool isNotEmpty() const               { return list.isNotEmpty(); }
  T *nth(int which)                     { return (T*)list.nth(which); }
  T const *nthC(int which) const        { return (T const*)list.nth(which); }
  T *first()                            { return (T*)list.first(); }
  T const *firstC() const               { return (T const*)list.first(); }
  T *last()                             { return (T*)list.last(); }
  T const *lastC() const                { return (T const*)list.last(); }

  // insertion
  void prepend(T *newitem)              { list.prepend(newitem); }
  void append(T *newitem)               { list.append(newitem); }
  void insertAt(T *newitem, int index)  { list.insertAt(newitem, index); }
  void insertSorted(T *newitem, Diff diff, void *extra=NULL)
    { list.insertSorted(newitem, (VoidDiff)diff, extra); }

  // removal
  T *removeAt(int index)                { return (T*)list.removeAt(index); }
outputCond([[[m4_dnl     // sobjlist
  void removeAll()                      { list.removeAll(); }
]]], [[[m4_dnl           // objlist
  void deleteAt(int index)              { delete (T*)list.removeAt(index); }
  void deleteAll();
]]])m4_dnl

  // list-as-set: selectors
  int indexOf(T const *item) const      { return list.indexOf((void*)item); }
  int indexOfF(void *item) const        { return list.indexOfF((void*)item); }
  bool contains(T const *item) const    { return list.contains((void*)item); }

  // list-as-set: mutators
  bool prependUnique(T *newitem)        { return list.prependUnique(newitem); }
  bool appendUnique(T *newitem)         { return list.appendUnique(newitem); }
  void removeItem(T const *item)        { list.removeItem((void*)item); }    // whether the arg should be const is debatable..
  bool removeIfPresent(T const *item)   { return list.removeIfPresent((void*)item); }

  // complex modifiers
  void reverse()                                    { list.reverse(); }
  void insertionSort(Diff diff, void *extra=NULL)   { list.insertionSort((VoidDiff)diff, extra); }
  void mergeSort(Diff diff, void *extra=NULL)       { list.mergeSort((VoidDiff)diff, extra); }

  // and a related test
  bool isSorted(Diff diff, void *extra=NULL) const  { return list.isSorted((VoidDiff)diff, extra); }

  // multiple lists
  void concat(className &tail)                       { list.concat(tail.list); }
outputCond([[[m4_dnl    // sobjlist
  void appendAll(className const &tail)              { list.appendAll(tail.list); }
  className& operator= (className const &src)         { list = src.list; return *this; }
]]], [[[m4_dnl          // objlist
  // (we do *not* have operator= here, nor appendAll, since these are supposed to be owner lists)
]]])m4_dnl

  // equal items in equal positions
  bool equalAsLists(className const &otherList, Diff diff, void *extra=NULL) const
    { return list.equalAsLists(otherList.list, (VoidDiff)diff, extra); }

  // last-as-set: comparisons (NOT efficient)
  bool equalAsSets(className const &otherList, Diff diff, void *extra=NULL) const
    { return list.equalAsSets(otherList.list, (VoidDiff)diff, extra); }
  bool isSubsetOf(className const &otherList, Diff diff, void *extra=NULL) const
    { return list.isSubsetOf(otherList.list, (VoidDiff)diff, extra); }
  bool containsByDiff(T const *item, Diff diff, void *extra=NULL) const
    { return list.containsByDiff((void*)item, (VoidDiff)diff, extra); }

  // treating the pointer values themselves as the basis for comparison
  bool equalAsPointerLists(className const &otherList) const
    { return list.equalAsPointerLists(otherList.list); }
  bool equalAsPointerSets(className const &otherList) const
    { return list.equalAsPointerSets(otherList.list); }

outputCond([[[m4_dnl    // sobjlist
  // debugging: no invariants beyond VoidList
  void selfCheck() const                { list.selfCheck(); }
  
  // but export the additional checks for cases where they apply anyway
  void checkHeapDataPtrs() const        { list.checkHeapDataPtrs(); }
  void checkUniqueDataPtrs() const      { list.checkUniqueDataPtrs(); }
]]], [[[m4_dnl          // objlist
  // debugging: two additional invariants
  void selfCheck() const { 
    list.selfCheck();
    list.checkHeapDataPtrs();
    list.checkUniqueDataPtrs();
  }
]]])m4_dnl
};


outputCond(, [[[m4_dnl      // objlist
template <class T>
void ObjList<T>::deleteAll()
{
  while (!list.isEmpty()) {
    deleteAt(0);
  }
}


]]])m4_dnl
// for traversing the list and modifying it (nodes and/or structure)
// NOTE: no list-modification fns should be called on 'list' while this
//       iterator exists, and only one such iterator should exist for
//       any given list
template <class T>
class mutatorName {
  friend iterName<T>;

protected:
  VoidListMutator mut;       // underlying mutator

public:
  mutatorName[[[]]](className<T> &lst)     : mut(lst.list) { reset(); }
  ~mutatorName[[[]]]()                    {}

  void reset()                          { mut.reset(); }

  // iterator copying; safe *only* until one of the mutators modifies
  // the list structure (by inserting or removing), at which time all
  // other iterators might be in limbo
  mutatorName[[[]]](mutatorName const &obj)             : mut(obj.mut) {}
  mutatorName& operator=(mutatorName const &obj)  { mut = obj.mut;  return *this; }
    // requires that 'this' and 'obj' already refer to the same 'list'

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

outputCond(, [[[m4_dnl    // sobjlist
  void deleteIt()                       { delete (T*)mut.remove(); }
    // same as remove(), except item is deleted also

]]])m4_dnl
  // debugging
  void selfCheck() const                { return mut.selfCheck(); }
};

#define makeName(MUTATE_EACH_OBJLIST)(T, list, iter) \
  for(mutatorName<T> iter(list); !iter.isDone(); iter.adv())


// for traversing the list without modifying it (neither nodes nor structure)
// NOTE: no list-modification fns should be called on 'list' while this
//       iterator exists
template <class T>
class iterName {
protected:
  VoidListIter iter;      // underlying iterator

public:
  iterName[[[]]](className<T> const &list) : iter(list.list) {}
  ~iterName[[[]]]()                       {}

  void reset(className<T> const &list)   { iter.reset(list.list); }

  // iterator copying; generally safe
  iterName[[[]]](iterName const &obj)             : iter(obj.iter) {}
  iterName& operator=(iterName const &obj)  { iter = obj.iter;  return *this; }

  // but copying from a mutator is less safe; see above
  iterName[[[]]](mutatorName<T> &obj)             : iter(obj.mut) {}

  // iterator actions
  bool isDone() const                   { return iter.isDone(); }
  void adv()                            { iter.adv(); }
  T const *data() const                 { return (T const*)iter.data(); }
};

#define makeName(FOREACH_OBJLIST)(T, list, iter) \
  for(iterName<T> iter(list); !iter.isDone(); iter.adv())


#endif // includeLatch
