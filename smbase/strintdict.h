// strintdict.h
// dictionary of integers (or other types that fit into void*), indexed by
// string (case-sensitive)

// quarl 2006-05-31 initial version, based on strsobjdict.h

// (c) Scott McPeak, 2000

#ifndef STRINTDICT_H
#define STRINTDICT_H

#include "svdict.h"    // StringVoidDict


// since the dictionary does not own the pointed-to objects,
// it has the same constness model as StringVoidDict, namely
// that const means the *mapping* is constant but not the
// pointed-to objects

template <class T>
class StringIntDict {
public:     // types
  // 'foreach' iterator functions
  typedef bool (*ForeachFn)(string const &key, T value, void *extra);

  // external iterator
  class Iter {
  private:
    StringVoidDict::Iter iter;

  public:
    Iter(StringIntDict &dict) : iter(dict.dict) {}
    Iter(Iter const &obj) : DMEMB(iter) {}
    Iter& operator= (Iter const &obj) { CMEMB(iter); return *this; }

    bool isDone() const { return iter.isDone(); }
    Iter& next() { iter.next(); return *this; }

    string const &key() const { return iter.key(); }
    T &value() const { return (T &)iter.value(); }

    int private_getCurrent() const { return iter.private_getCurrent(); }
  };
  friend class Iter;

  class IterC {
  private:
    StringVoidDict::IterC iter;

  public:
    IterC(StringIntDict const &dict) : iter(dict.dict) {}
    IterC(IterC const &obj) : DMEMB(iter) {}
    IterC& operator= (IterC const &obj) { CMEMB(iter); return *this; }

    bool isDone() const { return iter.isDone(); }
    IterC& next() { iter.next(); return *this; }

    string const &key() const { return iter.key(); }
    T value() const { return (T )iter.value(); }

    int private_getCurrent() const { return iter.private_getCurrent(); }
  };
  friend class IterC;

private:    // data
  // underlying dictionary functionality
  StringVoidDict dict;

public:     // funcs
  StringIntDict() : dict() {}
  StringIntDict(StringIntDict const &obj) : dict(obj.dict) {}
  ~StringIntDict() {}

  // comparison and assignment use *pointer* comparison/assignment

  StringIntDict& operator= (StringIntDict const &obj)     { dict = obj.dict; return *this; }

  bool operator== (StringIntDict const &obj) const        { return dict == obj.dict; }
  NOTEQUAL_OPERATOR(StringIntDict)

  // due to similarity with StringVoidDict, see svdict.h for
  // details on these functions' interfaces

  // ------- selectors ---------
  int size() const                                     { return dict.size(); }

  bool isEmpty() const                                 { return dict.isEmpty(); }
  bool isNotEmpty() const                              { return !isEmpty(); }

  bool query(char const *key, T &value) const          { return dict.query(key, (void*&)value); }
  T queryf(char const *key) const                      { return (T)dict.queryf(key); }
  T queryif(char const *key) const                     { return (T)dict.queryif(key); }

  bool isMapped(char const *key) const                 { return dict.isMapped(key); }

  // -------- mutators -----------
  void add(char const *key, T value)                   { dict.add(key, (void*)value); }

  T modify(char const *key, T newValue)                { return (T)dict.modify(key, (void*)newValue); }

  T remove(char const *key)                            { return (T)dict.remove(key); }

  void empty()                                         { dict.empty(); }

  // -------- parallel interface for 'rostring' --------
  bool query(rostring key, T &value) const  { return query(key.c_str(), value); }
  T queryf(rostring key) const              { return queryf(key.c_str()); }
  T queryif(rostring key) const             { return queryif(key.c_str()); }
  bool isMapped(rostring key) const         { return isMapped(key.c_str()); }
  void add(rostring key, T value)           { dict.add(key, (void*)value); }
  T modify(rostring key, T newValue)        { return modify(key.c_str(), newValue); }
  T remove(rostring key)                    { return remove(key.c_str()); }

  // --------- iters -------------
  void foreach(ForeachFn func, void *extra=NULL) const
    { dict.foreach((StringVoidDict::ForeachFn)func, extra); }

  // debugging
  int private_getTopAddr() const { return dict.private_getTopAddr(); }
};


#endif // __STRINTDICT_H
