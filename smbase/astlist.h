// astlist.h
// owner list wrapper around VoidTailList
// name 'AST' is because the first application is in ASTs

#ifndef ASTLIST_H
#define ASTLIST_H

#include "vdtllist.h"     // VoidTailList

template <class T> class ASTListIter;

// a list which owns the items in it (will deallocate them), and
// has constant-time access to the last element
template <class T>
class ASTList {
private:
  friend class ASTListIter<T>;

protected:
  VoidTailList list;                    // list itself

private:
  ASTList(ASTList const &obj);          // not allowed

public:
  ASTList()                             : list() {}
  ~ASTList()                            { deleteAll(); }

  // stealing ctor; among other things, since &src->list is assumed to
  // point at 'this', this class can't have virtual functions
  ASTList(ASTList<T> *src)              : list(&src->list) {}
  void steal(ASTList<T> *src)           { deleteAll(); list.steal(&src->list); }

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

  // removal
  T *removeFirst()                      { return (T*)list.removeFirst(); }
  void deleteFirst()                    { delete (T*)list.removeFirst(); }
  void deleteAll();

  // list-as-set: selectors
  int indexOf(T const *item) const      { return list.indexOf((void*)item); }
  int indexOfF(void *item) const        { return list.indexOfF((void*)item); }
  bool contains(T const *item) const    { return list.contains((void*)item); }

  // list-as-set: mutators
  bool prependUnique(T *newitem)        { return list.prependUnique(newitem); }
  bool appendUnique(T *newitem)         { return list.appendUnique(newitem); }

  // debugging: two additional invariants
  void selfCheck() const                { list.selfCheck(); }
};


template <class T>
void ASTList<T>::deleteAll()
{
  while (!list.isEmpty()) {
    deleteFirst();
  }
}


template <class T>
class ASTListIter {
protected:
  VoidTailListIter iter;      // underlying iterator

public:
  ASTListIter(ASTList<T> const &list) : iter(list.list) {}
  ~ASTListIter()                       {}

  void reset(ASTList<T> const &list)   { iter.reset(list.list); }

  // iterator copying; generally safe
  ASTListIter(ASTListIter const &obj)             : iter(obj.iter) {}
  ASTListIter& operator=(ASTListIter const &obj)  { iter = obj.iter;  return *this; }

  // iterator actions
  bool isDone() const                   { return iter.isDone(); }
  void adv()                            { iter.adv(); }
  T const *data() const                 { return (T const*)iter.data(); }
};

#define FOREACH_ASTLIST(T, list, iter) \
  for(ASTListIter<T> iter(list); !iter.isDone(); iter.adv())


#endif // ASTLIST_H
