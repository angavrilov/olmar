// intstack.h            see license.txt for copyright and terms of use
// stack of ints

// quarl 2006-05-31 initial version based on sobjstack.h

#ifndef INTSTACK_H
#define INTSTACK_H

#include "intlist.h"    // IntList

template <class T>
class IntStack {
private:      // data
  // will implement the stack as a list, with prepend and removeAt(0)
  IntList<T> list;

public:       // funcs
  IntStack()                            : list() {}
  ~IntStack()                           {}

  int count() const                     { return list.count(); }
  bool isEmpty() const                  { return list.isEmpty(); }
  bool isNotEmpty() const               { return list.isNotEmpty(); }

  T top()                               { return list.first(); }

  // peek at nth item (linear time)
  T nth(int which)                      { return list.nth(which); }

  T pop()                               { return list.removeAt(0); }
  void delPop()                         { list.deleteAt(0); }
  void push(T item)                     { list.prepend(item); }
  void clear()                          { list.deleteAll(); }

  bool contains(T const item) const     { return list.contains((void*)item); }
};

#endif // INTSTACK_H
