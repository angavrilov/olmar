// rcptr.h
// a stab at a reference-counting pointer

// the object pointed-at must support this interface:
//   // increment reference count
//   void incRefCt();
//   
//   // decrement refcount, and if it becomes 0, delete yourself
//   void decRefCt();

#ifndef __RCPTR_H
#define __RCPTR_H

#include "typ.h"      // NULL

#if 0
  #include <stdio.h>    // printf, temporary
  #define DBG(fn) printf("%s(%p)\n", fn, ptr)
#else
  #define DBG(fn)
#endif

template <class T>
class RCPtr {
private:    // data
  T *ptr;                // the real pointer

private:    // funcs
  void inc() { DBG("inc"); if (ptr) { ptr->incRefCt(); } }
  void dec() { DBG("dec"); if (ptr) { ptr->decRefCt(); ptr=NULL; } }

public:     // funcs
  RCPtr(T *p = NULL) : ptr(p) { DBG("ctor"); inc(); }
  RCPtr(RCPtr const &obj) : ptr(obj.ptr) { DBG("cctor"); inc(); }
  ~RCPtr() { DBG("dtor"); dec(); }

  // point at something new (setting to NULL is an option)
  void operator= (T *p) { DBG("op=ptr"); dec(); ptr=p; inc(); }
  void operator= (RCPtr<T> const &obj)
    { DBG("op=obj"); dec(); ptr=obj.ptr; inc(); }

  // some operators that make Owner behave more or less like
  // a native C++ pointer.. note that some compilers to really
  // bad handling the "ambiguity", so the non-const versions
  // can be disabled at compile time
  operator T const * () const { DBG("opcT*"); return ptr; }
  T const & operator* () const { DBG("opc*"); return *ptr; }
  T const * operator-> () const { DBG("opc->"); return ptr; }

  #ifndef NO_REFCT_NONCONST
  operator T* () { DBG("opT*"); return ptr; }
  T& operator* () { DBG("op*"); return *ptr; }
  T* operator-> () { DBG("op->"); return ptr; }
  #endif

  // escape hatch for when operators flake out on us
  T *get() { DBG("get"); return ptr; }
  T const *getC() const { DBG("getC"); return ptr; }
  
  // sometimes, in performance-critical code, I need fine control
  // over the refcount operations; this lets me change 'ptr', the
  // assumption being I'll update the refct manually
  void setWithoutUpdateRefct(T *p) { ptr=p; }
};


#endif // __RCPTR_H
