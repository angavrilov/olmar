// owner.h
// a stab at an owner ptr abstraction

#ifndef __OWNER_H
#define __OWNER_H

#include "typ.h"      // NULL

#if 0
  #include <stdio.h>    // printf, temporary
  #define DBG(fn) printf("%s(%p)\n", fn, ptr)
#else
  #define DBG(fn)
#endif

template <class T>
class Owner {
private:    // data
  T *ptr;                // the real pointer

private:    // funcs
  Owner(Owner&);         // not allowed

public:     // funcs
  Owner(T *p = NULL) : ptr(p) { DBG("ctor"); }
  ~Owner() { DBG("dtor"); del(); }

  // take ownership (no transitive = here)
  void operator= (T *p) { DBG("op=ptr"); del(); ptr=p; }
  void operator= (Owner<T> &obj) { DBG("op=obj"); del(); ptr=obj.ptr; obj.ptr=NULL; }

  // release ownership
  T *xfr() { DBG("xfr"); T *temp = ptr; ptr = NULL; return temp; }

  // free
  void del() { DBG("del"); delete ptr; ptr = NULL; }    // relies on delete(NULL) being ok

  // some operators that make Owner behave more or less like
  // a native C++ pointer.. note that some compilers to really
  // bad handling the "ambiguity", so the non-const versions
  // can be disabled at compile time
  operator T const * () const { DBG("opcT*"); return ptr; }
  T const & operator* () const { DBG("opc*"); return *ptr; }
  T const * operator-> () const { DBG("opc->"); return ptr; }

  #ifndef NO_OWNER_NONCONST
  operator T* () { DBG("opT*"); return ptr; }
  T& operator* () { DBG("op*"); return *ptr; }
  T* operator-> () { DBG("op->"); return ptr; }
  #endif

  // escape hatch for when operators flake out on us
  T *get() { DBG("get"); return ptr; }
  T const *getC() const { DBG("getC"); return ptr; }
  
  // swaps are interesting because they don't require checking
  void swapWith(Owner<T> &obj) {
    T *tmp = ptr;
    ptr = obj.ptr;
    obj.ptr = tmp;
  }
};
   

template <class T>
void swap(Owner<T> &obj1, Owner<T> &obj2)
{
  obj1.swapWith(obj2);
}


#endif // __OWNER_H
