// strsobjdict.h            see license.txt for copyright and terms of use
// dictionary of *serf* pointers to objects, indexed by string (case-sensitive)
// (c) Scott McPeak, 2000

// created by copying strobjdict and modifying.. nonideal..

#ifndef __STRSOBJDICT_H
#define __STRSOBJDICT_H

#include "svdict.h"    // StringVoidDict


// since the dictionary does not own the pointed-to objects,
// it has the same constness model as StringVoidDict, namely
// that const means the *mapping* is constant but not the
// pointed-to objects

template <class T>
class StringSObjDict {
public:     // types
  // 'foreach' iterator functions
  typedef bool (*ForeachFn)(string const &key, T *value, void *extra);

  // external iterator
  class Iter {
  private:
    StringVoidDict::IterC iter;

  public:
    Iter(StringSObjDict const &dict) : iter(dict.dict) {}
    Iter(Iter const &obj) : DMEMB(iter) {}
    Iter& operator= (Iter const &obj) { CMEMB(iter); return *this; }

    bool isDone() const { return iter.isDone(); }
    Iter& next() { iter.next(); return *this; }

    string const &key() const { return iter.key(); }
    T *&value() const { return (T *&)iter.value(); }
  };
  friend class Iter;

private:    // data
  // underlying dictionary functionality
  StringVoidDict dict;

public:     // funcs
  StringSObjDict() : dict() {}
  StringSObjDict(StringSObjDict const &obj) : dict(obj.dict) {}
  ~StringSObjDict() {}

  // comparison and assignment use *pointer* comparison/assignment

  StringSObjDict& operator= (StringSObjDict const &obj)    { dict = obj.dict; return *this; }

  bool operator== (StringSObjDict const &obj) const        { return dict == obj.dict; }
  NOTEQUAL_OPERATOR(StringSObjDict)

  // due to similarity with StringVoidDict, see svdict.h for
  // details on these functions' interfaces

  // ------- selectors ---------
  int size() const                                     { return dict.size(); }

  bool isEmpty() const                                 { return dict.isEmpty(); }
  bool isNotEmpty() const                              { return !isEmpty(); }

  bool query(char const *key, T *&value) const         { return dict.query(key, (void*&)value); }
  T *queryf(char const *key) const                     { return (T*)dict.queryf(key); }
  T *queryif(char const *key) const                    { return (T*)dict.queryif(key); }

  bool isMapped(char const *key) const                 { return dict.isMapped(key); }

  // -------- mutators -----------
  void add(char const *key, T *value)                  { dict.add(key, value); }

  T *modify(char const *key, T *newValue)              { return (T*)dict.modify(key, newValue); }

  T *remove(char const *key)                           { return (T*)dict.remove(key); }

  void empty()                                         { dict.empty(); }

  // --------- iters -------------
  void foreach(ForeachFn func, void *extra=NULL) const
    { dict.foreach((StringVoidDict::ForeachFn)func, extra); }
};


#endif // __STRSOBJDICT_H
