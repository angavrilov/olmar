// array.h
// some array classes

#ifndef ARRAY_H
#define ARRAY_H

#include "xassert.h"      // xassert


// ------------------ GrowArray --------------------
template <class T>
class GrowArray {
private:     // data
  T *arr;                 // underlying array
  int sz;                 // # allocated entries in 'arr'

private:     // funcs
  void bc(int i) const;   // bounds-check an index

public:      // funcs
  GrowArray(int initSz);
  ~GrowArray();

  int size() const { return sz; }

  // element access
  T const& operator[] (int i) const   { bc(i); return arr[i]; }
  T      & operator[] (int i)         { bc(i); return arr[i]; }

  // set size, reallocating if old size is different; if the
  // array gets bigger, existing elements are preserved; if the
  // array gets smaller, elements are truncated
  void setSize(int newSz);

  // make sure there are at least 'minSz' elements in the array;
  void ensureAtLeast(int minSz)
    { if (minSz > sz) { setSize(minSz); } }

  // grab a read-only pointer to the raw array
  T const *getArray() const { return arr; }
  
  // make sure the given index is valid; if this requires growing,
  // do so by doubling the size of the array (repeatedly, if
  // necessary)
  void ensureIndexDoubler(int index);
  
  // set an element, using the doubler if necessary
  void setIndexDoubler(int index, T const &value)
    { ensureIndexDoubler(index); arr[index] = value; }
};


template <class T>
GrowArray<T>::GrowArray(int initSz)
{
  sz = initSz;
  if (sz > 0) {
    arr = new T[sz];
  }
  else {
    arr = NULL;
  }
}


template <class T>
GrowArray<T>::~GrowArray()
{
  if (arr) {
    delete[] arr;
  }
}


template <class T>
void GrowArray<T>::bc(int i) const
{
  xassert((unsigned)i < (unsigned)sz);
}


template <class T>
void GrowArray<T>::setSize(int newSz)
{
  if (newSz != sz) {
    // keep track of old
    int oldSz = sz;
    T *oldArr = arr;

    // make new
    sz = newSz;
    if (sz > 0) {
      arr = new T[sz];
    }
    else {
      arr = NULL;
    }

    // copy elements in common
    for (int i=0; i<sz && i<oldSz; i++) {
      arr[i] = oldArr[i];
    }

    // get rid of old
    if (oldArr) {
      delete[] oldArr;
    }
  }
}


template <class T>
void GrowArray<T>::ensureIndexDoubler(int index)
{                  
  if (sz-1 >= index) {
    return;
  }

  int newSz = sz;
  while (newSz-1 < index) {
    int prevSz = newSz;
    if (newSz == 0) {
      newSz = 1;
    }
    newSz = newSz*2;
    xassert(newSz > prevSz);   // otherwise overflow -> infinite loop
  }

  setSize(newSz);
}


// ---------------------- ArrayStack ---------------------
template <class T>
class ArrayStack : public GrowArray<T> {
private:
  int len;               // # of elts in the stack

public:
  ArrayStack(int initArraySize = 10)
    : GrowArray<T>(initArraySize),
      len(0)
    {}
  ~ArrayStack();

  void push(T const &val)
    { setIndexDoubler(len++, val); }
  T pop()
    { return operator[](--len); }
  T top() const
    { return operator[](len-1); }

  int length() const
    { return len; }
  bool isEmpty() const
    { return len==0; }
  bool isNotEmpty() const
    { return !isEmpty(); }

  void popMany(int ct)
    { len -= ct; xassert(len >= 0); }
};

template <class T>
ArrayStack<T>::~ArrayStack()
{}


#if 0      // in progress
// ------------------- ObjArrayStack -----------------
// an ArrayStack of owner pointers
template <class T>
class ObjArrayStack {
private:    // data
  ArrayStack<T*> arr;

public:     // funcs
  ObjArrayStack(int initArraySize = 10)
    : arr(initArraySize)
    {}
  ~ObjArrayStack() { deleteTopSeveral(length()); }

  void push(T *ptr)          { arr.push(ptr); }
  T *pop()                   { return arr.pop(); }
  T const *top() const       { return arr.top(); }

  int length() const         { return arr.length(); }
  bool isEmpty() const       { return arr.isEmpty(); }
  bool isNotEmpty() const    { return !isEmpty(); }
  #error in progress
};


template <class T>
void ObjArrayStack::deleteAll()
{
#endif // 0



#endif // ARRAY_H
