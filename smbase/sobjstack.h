// sobjstack.h            see license.txt for copyright and terms of use
// stack of objects, *not* owned by the stack

#ifndef SOBJSTACK_H
#define SOBJSTACK_H

#include "sobjlist.h"    // SObjList

template <class T>
class SObjStack {
private:      // data
  // will implement the stack as a list, with prepend and removeAt(0)
  SObjList<T> list;

public:       // funcs
  SObjStack()                           : list() {}
  ~SObjStack()                          {}

  int count() const                     { return list.count(); }
  bool isEmpty() const                  { return list.isEmpty(); }
  bool isNotEmpty() const               { return list.isNotEmpty(); }

  T const *topC() const                 { return list.firstC(); }
  T *top()                              { return list.first(); }

  T *pop()                              { return list.removeAt(0); }
  void push(T *item)                    { list.prepend(item); }

  bool contains(T const *item) const    { return list.contains((void*)item); }
};

#endif // SOBJSTACK_H
