// sastlist.h            see license.txt for copyright and terms of use
// non-owner list wrapper around VoidTailList
// FIX: this file is almost an identical copy of astlist.h
// name 'AST' is because the first application is in ASTs

#ifndef SASTLIST_H
#define SASTLIST_H

#include "vdtllist.h"     // VoidTailList

template <class T> class SASTListIter;
template <class T> class SASTListIterNC;

// a list which owns the items in it (will deallocate them), and
// has constant-time access to the last element
template <class T>
class SASTList {
private:
  friend class SASTListIter<T>;        
  friend class SASTListIterNC<T>;

protected:
  VoidTailList list;                    // list itself

private:
  // FIX: we should diverge from ASTList here and follow SObjList
  // instead, since we are not an owning list, and allow shallow
  // copies.  However, this requires implementing shallow clonding for
  // VoidTailList
  SASTList(SASTList const &obj)         : list(obj.list) {}
  SASTList& operator= (SASTList const &src)         { list = src.list; return *this; }

public:
  SASTList()                             : list() {}
  ~SASTList()                            { removeAll_dontDelete(); }

  // ctor to make singleton list; often quite useful
  SASTList(T *elt)                       : list() { prepend(elt); }

  // stealing ctor; among other things, since &src->list is assumed to
  // point at 'src', this class can't have virtual functions;
  // these ctors delete 'src'
  SASTList(SASTList<T> *src)              : list(&src->list) {}
  void steal(SASTList<T> *src)           { removeAll_dontDelete(); list.steal(&src->list); }

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
  void concat(SASTList<T> &tail)         { list.concat(tail.list); }

  // removal
  T *removeFirst()                      { return (T*)list.removeFirst(); }
  T *removeLast()                       { return (T*)list.removeLast(); }
  T *removeAt(int index)                { return (T*)list.removeAt(index); }
  void removeItem(T *item)              { list.removeItem((void*)item); }
  
  // this one is awkwardly named to remind the user that it's
  // contrary to the usual intent of this class
  void removeAll_dontDelete()           { return list.removeAll(); }

  // deletion
  void deleteFirst()                    { delete (T*)list.removeFirst(); }
  void deleteAll();

  // list-as-set: selectors
  int indexOf(T const *item) const      { return list.indexOf((void*)item); }
  int indexOfF(T const *item) const     { return list.indexOfF((void*)item); }
  bool contains(T const *item) const    { return list.contains((void*)item); }

  // list-as-set: mutators
  bool prependUnique(T *newitem)        { return list.prependUnique(newitem); }
  bool appendUnique(T *newitem)         { return list.appendUnique(newitem); }

  // debugging: two additional invariants
  void selfCheck() const                { list.selfCheck(); }
};


template <class T>
void SASTList<T>::deleteAll()
{
  while (!list.isEmpty()) {
    deleteFirst();
  }
}


template <class T>
class SASTListIter {
protected:
  VoidTailListIter iter;      // underlying iterator

public:
  SASTListIter(SASTList<T> const &list) : iter(list.list) {}
  ~SASTListIter()                       {}

  void reset(SASTList<T> const &list)   { iter.reset(list.list); }

  // iterator copying; generally safe
  SASTListIter(SASTListIter const &obj)             : iter(obj.iter) {}
  SASTListIter& operator=(SASTListIter const &obj)  { iter = obj.iter;  return *this; }

  // iterator actions
  bool isDone() const                   { return iter.isDone(); }
  void adv()                            { iter.adv(); }
  T const *data() const                 { return (T const*)iter.data(); }
};

#define FOREACH_SASTLIST(T, list, iter) \
  for(SASTListIter<T> iter(list); !iter.isDone(); iter.adv())


// version of the above, but for non-const-element traversal
template <class T>
class SASTListIterNC {
protected:
  VoidTailListIter iter;      // underlying iterator

public:
  SASTListIterNC(SASTList<T> &list)      : iter(list.list) {}
  ~SASTListIterNC()                     {}

  void reset(SASTList<T> &list)         { iter.reset(list.list); }

  // iterator copying; generally safe
  SASTListIterNC(SASTListIterNC const &obj)             : iter(obj.iter) {}
  SASTListIterNC& operator=(SASTListIterNC const &obj)  { iter = obj.iter;  return *this; }

  // iterator actions
  bool isDone() const                   { return iter.isDone(); }
  void adv()                            { iter.adv(); }
  T *data() const                       { return (T*)iter.data(); }
  
  // iterator mutation; use with caution
  void setDataLink(T *newData)          { iter.setDataLink((void*)newData); }
};

#define FOREACH_SASTLIST_NC(T, list, iter) \
  for(SASTListIterNC<T> iter(list); !iter.isDone(); iter.adv())


// this function is somewhat at odds with the nominal purpose
// of SASTLists, but I need it in a weird situation so ...
template <class T>
SASTList<T> *shallowCopy(SASTList<T> *src)
{
  SASTList<T> *ret = new SASTList<T>;
  FOREACH_SASTLIST_NC(T, *src, iter) {
    ret->append(iter.data());
  }
  return ret;
}


#endif // SASTLIST_H
